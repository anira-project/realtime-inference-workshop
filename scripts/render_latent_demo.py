# /// script
# requires-python = ">=3.10"
# dependencies = ["numpy", "onnxruntime", "soundfile"]
# ///
"""Render the latent demo for step 10: the chords of render_demo.py through the encoder
and the decoder, once as they are and once with an offset on every latent dimension (OFFSETS).

The split path is step 10's: the encoder per block of 2048 samples, an offset added to
the 8 latents, the decoder per frame, each with its state carried by hand. The middle
pass is kept and looped as in render_demo.py, so the files line up with demo_input.wav.

Usage:
    uv run scripts/render_latent_demo.py [--out slides/assets/audio] [--measure]
"""

import argparse
import pathlib
import sys

import numpy as np
import onnxruntime as ort
import soundfile as sf

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from render_demo import (BEAT, BLOCK, FADE, LOOP_BEATS, PASSES, ROOT, SAMPLE_RATE,  # noqa: E402
                         latency, seamless, synthesize)

MODELS = ROOT / "steps/common/assets/models/rave"

# The offsets on the listening slide: one draw of every latent in ±3, picked by ear
OFFSETS = [1.6, 2.8, 2.3, 0.7, -2.0, 2.7, -2.9, -1.2]
# The shift on the map slide: only its two axes, z0 (louder) and z2 (brighter)
MAP_OFFSETS = [1.5, 0, 2.5, 0, 0, 0, 0, 0]


def run_split(signal, offsets):
    """Encoder, offset on z, decoder: block by block, both states carried."""
    enc = ort.InferenceSession(str(MODELS / "model_encoder.onnx"), providers=["CPUExecutionProvider"])
    dec = ort.InferenceSession(str(MODELS / "model_decoder.onnx"), providers=["CPUExecutionProvider"])
    enc_state = np.zeros(enc.get_inputs()[1].shape, dtype=np.float32)
    dec_state = np.zeros(dec.get_inputs()[1].shape, dtype=np.float32)
    offset = np.asarray(offsets, dtype=np.float32).reshape(1, 8, 1)
    out = []
    n = len(signal) // BLOCK * BLOCK
    for begin in range(0, n, BLOCK):
        block = signal[begin:begin + BLOCK].astype(np.float32).reshape(1, 1, BLOCK)
        z, enc_state = enc.run(None, {"audio": block, "state_in": enc_state})
        audio, dec_state = dec.run(None, {"latents": z + offset, "state_in": dec_state})
        out.append(audio.ravel())
    return np.concatenate(out)


def measure(audio):
    """Loudness (RMS, dB) and the spectral centroid (Hz), over the whole loop."""
    rms = 20 * np.log10(np.sqrt(np.mean(audio ** 2)) + 1e-12)
    spectrum = np.abs(np.fft.rfft(audio * np.hanning(len(audio))))
    freqs = np.fft.rfftfreq(len(audio), 1 / SAMPLE_RATE)
    return rms, float((freqs * spectrum).sum() / spectrum.sum())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=pathlib.Path, default=ROOT / "slides/assets/audio")
    parser.add_argument("--measure", action="store_true", help="print loudness and centroid per offset, write nothing")
    args = parser.parse_args()

    loop = int(LOOP_BEATS * BEAT * SAMPLE_RATE)
    dry_all = synthesize(PASSES + 1)
    lag = latency(MODELS / "model_forward.onnx")
    begin = loop

    def cut(audio):
        return seamless(audio[begin + lag:begin + lag + loop + FADE])

    variants = {"none": [0] * 8}
    if args.measure:
        for d in (0, 1, 2, 3):
            for v in (-1.0, 1.0):
                o = [0.0] * 8; o[d] = v
                variants[f"z{d} {v:+.0f}"] = o
    else:
        variants["offset"] = OFFSETS
        variants["map"] = MAP_OFFSETS

    results = {name: cut(run_split(dry_all, o)) for name, o in variants.items()}
    base_rms, base_c = measure(results["none"])
    for name, audio in results.items():
        rms, c = measure(audio)
        print(f"{name:7s} rms {rms:6.1f} dB ({rms - base_rms:+5.1f})  centroid {c:7.0f} Hz ({c / base_c:4.2f}x)  peak {np.abs(audio).max():.2f}")
    if args.measure:
        return
    files = {"none": "demo_split.wav", "offset": "demo_split_offset.wav", "map": "demo_split_map.wav"}
    peak = max(np.abs(a).max() for a in results.values())
    gain = min(1.0, 0.95 / peak)
    for name, audio in results.items():
        sf.write(args.out / files[name], audio * gain, SAMPLE_RATE, subtype="PCM_16")
    print(f"written to {args.out}, gain {gain:.2f}")


if __name__ == "__main__":
    main()
