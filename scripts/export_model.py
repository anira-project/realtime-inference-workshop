# /// script
# requires-python = ">=3.11,<3.13"
# dependencies = ["torch==2.5.1", "numpy<2", "onnx", "onnxruntime"]
# ///
"""Export the model the slides show for the C++ steps: one TorchScript file and three ONNX files.

Source: the Intelligent Instruments Lab's magnetic resonator piano model,
mrp_strengjavera_b2048_r44100_z16.ts (huggingface.co/Intelligent-Instruments-Lab/rave-models,
CC BY-NC 4.0). 44.1 kHz, 2048 samples per block, one channel, 8 latents.

  model.pt              audio[1,1,2048] -> audio[1,1,2048]          state inside, reset_state()
  model_forward.onnx    audio[1,1,2048], state_in[1,E+D] -> audio_out[1,1,2048], state_out[1,E+D]
  model_encoder.onnx    audio[1,1,2048], state_in[1,E]   -> latents[1,8,1],      state_out[1,E]
  model_decoder.onnx    latents[1,8,1],  state_in[1,D]   -> audio[1,1,2048],     state_out[1,D]

The streaming TorchScript model keeps its convolution caches as buffers updated in place, which
ONNX cannot express. So the model is re-run from the TorchScript weights with every cache passed
in and returned as one flat state tensor (forward's state is the encoder's followed by the
decoder's). A zero state is a fresh stream. The .pt keeps that state in a buffer instead: audio in,
audio out, as LibTorch can hold state and ONNX cannot.

Deterministic: the Realtime Audio Variational autoEncoder's encode() samples the posterior and
decode() fills the cropped latent dimensions with noise. Here the encoder takes the posterior mean and the decoder fills with zeros
(the PCA mean), so every engine gives the same output and the C++ steps can check against a
reference.

Usage:
    uv run scripts/export_model.py path/to/mrp_strengjavera_b2048_r44100_z16.ts [--out models]
"""
import argparse
import pathlib
import re

import numpy as np
import torch
import torch.nn.functional as F


def conv_args(module, transpose):
    """stride / dilation baked into the TorchScript code of one conv instance."""
    pattern = r"conv_transpose1d\(x, weight, None, \[(\d+)\], \[0\], \[0\], 1, \[(\d+)\]\)" if transpose \
        else r"conv1d\(x1, weight, bias, \[(\d+)\], \[0\], \[(\d+)\]\)"
    match = re.search(pattern, module.code)
    if match is None:
        raise RuntimeError(f"unsupported conv code:\n{module.code}")
    return int(match.group(1)), int(match.group(2))


class Interpreter:
    """Runs a TorchScript cached_conv module tree functionally; caches come from and go to a flat state."""

    def __init__(self):
        self.shapes = []
        self.collect = False

    def begin(self, state, collect=False):
        self.state, self.offset, self.new_state, self.collect = state, 0, [], collect

    def end(self):
        return torch.cat(self.new_state, 1) if self.new_state else torch.zeros(1, 0)

    def take(self, shape):
        if self.collect:
            self.shapes.append(tuple(shape))
            return torch.zeros(1, *shape)
        n = int(np.prod(shape))
        piece = self.state[:, self.offset:self.offset + n].reshape(1, *shape)
        self.offset += n
        return piece

    def put(self, tensor):
        self.new_state.append(tensor.reshape(1, -1))

    @property
    def size(self):
        return sum(int(np.prod(s)) for s in self.shapes)

    def run(self, mod, x):
        kind = mod.original_name
        if kind in ("CachedSequential", "DilatedUnit", "EncoderV2"):
            target = mod if kind == "CachedSequential" else mod.net
            for _, child in target.named_children():
                x = self.run(child, x)
            return x
        if kind == "CachedPadding1d":
            padding = int(mod.padding)
            if padding == 0:
                return x
            pad = self.take((x.shape[1], padding))
            x = torch.cat([pad, x], -1)
            self.put(x[..., -padding:])
            return x[..., :-padding] if bool(mod.crop) else x
        if kind == "CachedConv1d":
            x = self.run(mod.downsampling_delay, x)
            x = self.run(mod.cache, x)
            stride, dilation = conv_args(mod, False)
            return F.conv1d(x, mod.weight, mod.bias, stride, 0, dilation)
        if kind == "CachedConvTranspose1d":
            stride, dilation = conv_args(mod, True)
            y = F.conv_transpose1d(x, mod.weight, None, stride, 0, 0, 1, dilation)
            c = int(mod.cache.shape[-1])
            cache = self.take((y.shape[1], c))
            head = y[..., :c] + cache
            self.put(y[..., -c:])
            y = torch.cat([head, y[..., c:]], -1)[..., :-c]
            return y + mod.bias.unsqueeze(-1) if mod.bias is not None else y
        if kind == "Snake":
            return x + torch.reciprocal(mod.alpha + 1e-9) * torch.sin(mod.alpha * x) ** 2
        if kind == "LeakyReLU":
            return F.leaky_relu(x, 0.2)
        if kind == "Identity":
            return x
        if kind == "AdaptiveInstanceNormalization":
            if bool(mod.num_update_x) and bool(mod.num_update_y):
                raise RuntimeError("AdaIN style transfer is active; not supported")
            return x
        if kind == "Residual":
            branches = list(mod.aligned.branches.children())
            paddings = list(mod.aligned.paddings.children())
            net = self.run(branches[0], self.run(paddings[0], x))
            res = self.run(branches[1], self.run(paddings[1], x))
            return net + res
        raise RuntimeError(f"unsupported module {kind}")


def reverse_half(x):  # rave.pqmf.reverse_half
    mask = torch.ones_like(x)
    mask[:, 1::2, ::2] = -1
    return x * mask


class StreamingRave(torch.nn.Module):
    def __init__(self, ts):
        super().__init__()
        self.ts = ts
        self.latent_size = int(ts.latent_size)
        self.full_latent_size = int(ts.full_latent_size)
        self.block = int(ts.encode_params[3])
        self.sample_rate = int(ts.sr)
        self.register_buffer("latent_pca", ts.latent_pca.detach().clone())
        self.register_buffer("latent_mean", ts.latent_mean.detach().clone())
        if ts.encoder.original_name != "VariationalEncoder":
            raise RuntimeError(f"unsupported encoder {ts.encoder.original_name}")
        # One state slot per cache, in traversal order (filled by dry runs).
        self.enc, self.dec = Interpreter(), Interpreter()
        with torch.no_grad():
            self.enc.begin(None, collect=True)
            self.encoder_raw(torch.zeros(1, 1, self.block))
            self.dec.begin(None, collect=True)
            self.decoder_raw(torch.zeros(1, self.full_latent_size, 1))

    def encoder_raw(self, audio):
        pqmf = self.ts.pqmf
        x = reverse_half(self.enc.run(pqmf.forward_conv, audio)) if int(pqmf.n_band) > 1 else audio
        return self.enc.run(self.ts.encoder.encoder, x)

    def encode(self, audio, state):
        """Posterior mean, PCA-rotated, cropped to the exported latents."""
        self.enc.begin(state)
        mean, _ = torch.chunk(self.encoder_raw(audio), 2, 1)
        z = F.conv1d(mean - self.latent_mean.unsqueeze(-1), self.latent_pca.unsqueeze(-1))
        return z[:, :self.latent_size], self.enc.end()

    def decoder_raw(self, z):
        x = self.dec.run(self.ts.decoder.net, z)
        if bool(self.ts.decoder.amplitude_modulation):
            x, amplitude = torch.split(x, x.shape[1] // 2, 1)
            x = x * torch.sigmoid(amplitude)
        return self.pqmf_inverse(torch.tanh(x))

    def pqmf_inverse(self, x):
        pqmf = self.ts.pqmf
        if int(pqmf.n_band) == 1:
            return x
        m = int(pqmf.hk.shape[0])
        x = self.dec.run(pqmf.inverse_conv, reverse_half(x)) * m
        x = torch.flip(x, [1]).permute(0, 2, 1)
        x = x.reshape(x.shape[0], x.shape[1], -1, m).permute(0, 2, 1, 3)
        return x.reshape(x.shape[0], x.shape[1], -1)

    def decode(self, z, state, noise=None):
        """Cropped dims filled with zeros (deterministic) unless noise is given."""
        if noise is None:
            noise = torch.zeros(1, self.full_latent_size - self.latent_size, z.shape[-1])
        z = torch.cat([z, noise], 1)
        z = F.conv1d(z, self.latent_pca.T.unsqueeze(-1)) + self.latent_mean.unsqueeze(-1)
        self.dec.begin(state)
        audio = self.decoder_raw(z)
        return audio, self.dec.end()


class Encoder(torch.nn.Module):
    def __init__(self, rave): super().__init__(); self.rave = rave
    def forward(self, audio, state_in): return self.rave.encode(audio, state_in)


class Decoder(torch.nn.Module):
    def __init__(self, rave): super().__init__(); self.rave = rave
    def forward(self, latents, state_in): return self.rave.decode(latents, state_in)


class Forward(torch.nn.Module):
    def __init__(self, rave): super().__init__(); self.rave = rave

    def forward(self, audio, state_in):
        e = self.rave.enc.size
        z, enc_state = self.rave.encode(audio, state_in[:, :e])
        out, dec_state = self.rave.decode(z, state_in[:, e:])
        return out, torch.cat([enc_state, dec_state], 1)


class Stateful(torch.nn.Module):
    """Forward with the state in a buffer: audio in, audio out, for LibTorch."""

    def __init__(self, forward, size):
        super().__init__()
        self.inner = forward
        self.register_buffer("state", torch.zeros(1, size))

    def reset_state(self) -> torch.Tensor:
        """Clears the state, for prepare(). Returns it because a traced method has to return something."""
        self.state.zero_()
        return self.state

    def forward(self, audio):
        out, new_state = self.inner(audio, self.state)
        self.state.copy_(new_state)
        return out


def check_against_torchscript(ts, rave, blocks=40):
    """Streams the same signal and noise through TorchScript's own modules and the re-implementation."""
    torch.manual_seed(0)
    ref = ts
    enc_state, dec_state = torch.zeros(1, rave.enc.size), torch.zeros(1, rave.dec.size)
    # The TorchScript caches start at zero too; reset them so both start from a fresh stream.
    for name, buffer in ref.named_buffers():
        if name.endswith(".pad") or name.endswith(".cache"):
            buffer.zero_()
    enc_err = dec_err = 0.0
    with torch.no_grad():
        for _ in range(blocks):
            audio = torch.randn(1, 1, rave.block) * 0.1
            expected = ref.encoder.encoder(ref.pqmf(audio))
            rave.enc.begin(enc_state)
            got = rave.encoder_raw(audio)
            enc_state = rave.enc.end()
            enc_err = max(enc_err, float((got - expected).abs().max()))

            z = torch.randn(1, rave.latent_size, 1) * 1.5
            noise = torch.randn(1, rave.full_latent_size - rave.latent_size, 1)
            full = F.conv1d(torch.cat([z, noise], 1), ref.latent_pca.T.unsqueeze(-1)) + ref.latent_mean.unsqueeze(-1)
            expected = ref.pqmf.inverse(ref.decoder(full))
            got, dec_state = rave.decode(z, dec_state, noise)
            dec_err = max(dec_err, float((got - expected).abs().max()))
    return enc_err, dec_err


def export_onnx(module, args, path, input_names, output_names):
    with torch.no_grad():
        torch.onnx.export(module, args, str(path), input_names=input_names, output_names=output_names,
                          opset_version=17, dynamo=False)


def export_torchscript(stateful, block, path):
    with torch.no_grad():
        traced = torch.jit.trace_module(stateful, {"forward": (torch.zeros(1, 1, block),), "reset_state": ()},
                                        check_trace=False)
        traced.state.zero_()  # tracing ran a block; save the state a fresh stream starts from
    frozen = torch.jit.freeze(traced, preserved_attrs=["reset_state"])
    frozen.save(str(path))


def stream_onnx(path, first_name, blocks, state_size):
    import onnxruntime as ort
    options = ort.SessionOptions()
    options.intra_op_num_threads = 1
    session = ort.InferenceSession(str(path), options, providers=["CPUExecutionProvider"])
    state = np.zeros((1, state_size), dtype=np.float32)
    out = []
    for block in blocks:
        result, state = session.run(None, {first_name: block, "state_in": state})
        out.append(result)
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("model", type=pathlib.Path)
    parser.add_argument("--out", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parent.parent / "models")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)

    ts = torch.jit.load(str(args.model)).eval()
    rave = StreamingRave(torch.jit.load(str(args.model)).eval()).eval()
    L, B, E, D = rave.latent_size, rave.block, rave.enc.size, rave.dec.size
    print(f"{rave.sample_rate} Hz, block {B}, latents {L} (of {rave.full_latent_size}), "
          f"state: encoder {E} floats ({len(rave.enc.shapes)} caches), decoder {D} ({len(rave.dec.shapes)} caches)")

    enc_err, dec_err = check_against_torchscript(ts, rave)
    print(f"re-implementation vs. TorchScript: encoder {enc_err:.2e}, decoder {dec_err:.2e}")
    if max(enc_err, dec_err) > 1e-4:
        raise SystemExit("mismatch against the TorchScript model")

    encoder, decoder, forward = Encoder(rave).eval(), Decoder(rave).eval(), Forward(rave).eval()
    paths = {name: args.out / f"model_{name}.onnx" for name in ("forward", "encoder", "decoder")}
    export_onnx(forward, (torch.zeros(1, 1, B), torch.zeros(1, E + D)), paths["forward"],
                ["audio", "state_in"], ["audio_out", "state_out"])
    export_onnx(encoder, (torch.zeros(1, 1, B), torch.zeros(1, E)), paths["encoder"],
                ["audio", "state_in"], ["latents", "state_out"])
    export_onnx(decoder, (torch.zeros(1, L, 1), torch.zeros(1, D)), paths["decoder"],
                ["latents", "state_in"], ["audio", "state_out"])
    pt_path = args.out / "model.pt"
    export_torchscript(Stateful(forward, E + D).eval(), B, pt_path)

    # Every export against the PyTorch re-implementation, streaming a chord-like signal from a zero state.
    t = np.arange(16 * B) / rave.sample_rate
    signal = sum(0.15 * np.sin(2 * np.pi * f * t) for f in (220.0, 277.2, 329.6)).astype(np.float32)
    blocks = [signal[i * B:(i + 1) * B].reshape(1, 1, B) for i in range(16)]

    state, ref_audio, ref_latents = torch.zeros(1, E + D), [], []
    enc_state = torch.zeros(1, E)
    with torch.no_grad():
        for block in blocks:
            out, state = forward(torch.from_numpy(block), state)
            ref_audio.append(out.numpy())
            z, enc_state = encoder(torch.from_numpy(block), enc_state)
            ref_latents.append(z.numpy())

    pt = torch.jit.load(str(pt_path)).eval()
    with torch.no_grad():
        pt_audio = [pt(torch.from_numpy(b)).numpy() for b in blocks]
        pt.reset_state()
        pt_again = [pt(torch.from_numpy(b)).numpy() for b in blocks]
    onnx_audio = stream_onnx(paths["forward"], "audio", blocks, E + D)
    onnx_latents = stream_onnx(paths["encoder"], "audio", blocks, E)
    onnx_chain = stream_onnx(paths["decoder"], "latents", onnx_latents, D)

    worst = lambda a, b: max(float(np.abs(x - y).max()) for x, y in zip(a, b))
    errors = {
        "model.pt": worst(pt_audio, ref_audio),
        "model.pt after reset_state": worst(pt_again, pt_audio),
        "model_forward.onnx": worst(onnx_audio, ref_audio),
        "model_encoder.onnx (latents)": worst(onnx_latents, ref_latents),
        "encoder -> decoder.onnx": worst(onnx_chain, ref_audio),
    }
    peak = float(np.abs(np.concatenate(ref_audio, -1)).max())
    for name, error in errors.items():
        print(f"  {name:30} max abs diff {error:.2e}")
    print(f"  output peak {peak:.3f}")
    if max(errors.values()) > 1e-4:
        raise SystemExit("export mismatch")

    for path in (pt_path, *paths.values()):
        print(f"  {path.name:20} {path.stat().st_size / 1e6:6.1f} MB")
    print(f"ENCODER_STATE_SIZE={E} DECODER_STATE_SIZE={D} FORWARD_STATE_SIZE={E + D}")


if __name__ == "__main__":
    main()
