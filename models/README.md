# Models

The model files are **not in git**. They are downloaded from this repository's
own release at configure time, into this folder:

```
model.pt             127 MB   TorchScript, for LibTorch: audio in, audio out,
                              state kept inside the model, reset_state()
model_forward.onnx   126 MB   the same model for ONNX Runtime: audio plus the
                              state in, audio plus the new state out — here the
                              state is the caller's job
model_encoder.onnx    64 MB   audio + state in, 8 latents + state out
model_decoder.onnx    62 MB   8 latents + state in, audio + state out
```

All take 2048 samples (or one latent frame) per call, 44.1 kHz, one channel.
The state is one flat tensor per model, every convolution cache in it:

| model | inputs → outputs |
|---|---|
| `model_forward.onnx` | `audio[1,1,2048]`, `state_in[1,63488]` → `audio_out[1,1,2048]`, `state_out[1,63488]` |
| `model_encoder.onnx` | `audio[1,1,2048]`, `state_in[1,34304]` → `latents[1,8,1]`, `state_out[1,34304]` |
| `model_decoder.onnx` | `latents[1,8,1]`, `state_in[1,29184]` → `audio[1,1,2048]`, `state_out[1,29184]` |

Feed every `state_out` back in as the next call's `state_in`; zeros start a new
stream. Forward's state is the encoder's followed by the decoder's, so
encoder → decoder gives the same audio as forward.

## Where they come from

The Intelligent Instruments Lab's magnetic resonator piano model,
`mrp_strengjavera_b2048_r44100_z16.ts`
([huggingface.co/Intelligent-Instruments-Lab/rave-models](https://huggingface.co/Intelligent-Instruments-Lab/rave-models),
CC BY-NC 4.0). [`scripts/export_model.py`](../scripts/export_model.py) re-runs it
from the TorchScript weights with every cache passed in and out, exports the
four files, and checks them against each other:

```bash
uv run scripts/export_model.py path/to/mrp_strengjavera_b2048_r44100_z16.ts
uv run scripts/make_test_data.py   # steps/common/assets, from model_forward.onnx
```

The exports are deterministic: the encoder takes the posterior mean and the
decoder fills the cropped latent dimensions with zeros, where the Realtime
Audio Variational autoEncoder samples noise. So LibTorch, ONNX Runtime and the
Python reference give the same output, and the steps can check against it.

## Why not in git

380 MB of weights make every clone slow, and Git LFS has a bandwidth quota
(1 GB per month on the free plan) that a roomful of people runs through in an
hour. Release assets have neither problem.

## How the download works

`cmake/models.cmake` fetches each file from the release tagged
`workshop-model` and checks its SHA-256. Files that are already here and intact are
left alone, so a second configure run needs no network.

A build directory configured against an older release still has the old file
names cached; on its next configure (which `cmake --build` triggers by itself
after a pull) it downloads the new files and switches to them.

If the download fails, CMake says which file and where to get it. To use copies
from somewhere else:

```bash
cmake -S . -B build -DWORKSHOP_MODEL=/path/to/model.pt \
                    -DWORKSHOP_ONNX_MODEL=/path/to/model_forward.onnx
```
