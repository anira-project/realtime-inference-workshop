# Models

The model files are **not in git**. They are downloaded from this repository's
own release at configure time, into this folder, next to the audio in
[`../audio/`](../audio/):

```
rave/model_forward.onnx   126 MB   audio plus the state in, audio plus the new
                                   state out: the state is the caller's job
rave/model_encoder.onnx    64 MB   audio + state in, 8 latents + state out
rave/model_decoder.onnx    62 MB   8 latents + state in, audio + state out
handtracking/              the four hand tracking models of step 11
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

**Latency: 2224 samples** (50.4 ms): the output answers the input this much
later. 1696 come from the causal network (strides and transposed
convolutions), 528 from the filter bank. `scripts/export_model.py` computes it
from the export; the C++ steps carry it as `k_model.m_latency`.

## Where they come from

The Intelligent Instruments Lab's magnetic resonator piano model,
`mrp_strengjavera_b2048_r44100_z16.ts`
([huggingface.co/Intelligent-Instruments-Lab/rave-models](https://huggingface.co/Intelligent-Instruments-Lab/rave-models),
CC BY-NC 4.0). [`scripts/export_model.py`](../scripts/export_model.py) re-runs it
from the TorchScript weights with every cache passed in and out, exports the
files, and checks them against each other:

```bash
uv run scripts/export_model.py path/to/mrp_strengjavera_b2048_r44100_z16.ts
uv run scripts/make_test_data.py   # steps/common/assets, from model_forward.onnx
```

The exports are deterministic: the encoder takes the posterior mean and the
decoder fills the cropped latent dimensions with zeros, where the Realtime
Audio Variational autoEncoder samples noise. So ONNX Runtime and the Python
reference give the same output, and the steps can check against it.

## Why not in git

380 MB of weights make every clone slow, and Git LFS has a bandwidth quota
(1 GB per month on the free plan) that a roomful of people runs through in an
hour. Release assets have neither problem.

## How the download works

[`cmake/models.cmake`](../../../../cmake/models.cmake) fetches the files as
three archives from the release tagged `workshop-model`, one per set of steps,
and only those the enabled steps need:

```
workshop-models-forward-2.zip   rave/model_forward.onnx                     steps 1 to 9
workshop-models-split.zip       rave/model_encoder.onnx, model_decoder.onnx steps 10, 11
workshop-models-hands.zip       handtracking/, four models                  step 11
```

Each archive and each file in it is checked against its SHA-256, and a failed
download is retried. The archive goes to the build folder and is deleted once
its files are here. Files that are already here and intact are left alone, so a
second configure needs no network. To use a model file from somewhere else:

```bash
cmake --preset release -DWORKSHOP_ONNX_MODEL=/path/to/model_forward.onnx
```
