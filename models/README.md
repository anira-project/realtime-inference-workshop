# Models

The model files are **not in git**. They are downloaded from this repository's
own release at configure time, into this folder:

```
forward_stateful.pt    65 MB   TorchScript, for LibTorch: audio in, audio out,
                               state kept inside the model
forward.onnx + .data   65 MB   the same model for ONNX Runtime: audio plus 40
                               state tensors in, audio plus 40 new ones out —
                               here the state is the caller's job
```

Both take 2048 samples per forward pass, 48 kHz, one channel.

## Why not in git

124 MB of weights make every clone slow, and Git LFS has a bandwidth quota
(1 GB per month on the free plan) that a roomful of people runs through in an
hour. Release assets have neither problem.

## How the download works

`cmake/models.cmake` fetches each file from the release tagged
`models-v1` and checks its SHA-256. Files that are already here and intact are
left alone, so a second configure run needs no network.

If the download fails, CMake says which file and where to get it. To use copies
from somewhere else:

```bash
cmake -S . -B build -DWORKSHOP_MODEL=/path/to/forward_stateful.pt \
                    -DWORKSHOP_ONNX_MODEL=/path/to/forward.onnx
```

`forward.onnx.data` has to sit next to `forward.onnx` under exactly that name —
the ONNX file references it by name.

## Where they come from

Both are exports of the pqmf streaming model from `seqsynth`. The TorchScript
one is built from the ONNX one by the scripts in `tmp/` (see `tmp/README.md`),
which also verify that the two agree with each other and with the Python
reference.
