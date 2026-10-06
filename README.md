# Real-Time Neural Inference Workshop

From an exported neural model to a real-time-safe audio plugin, in C++.

The workshop starts naive and runs into every problem on purpose: each step fixes the previous step's problem and exposes the next one, until [anira](https://github.com/anira-project/anira) replaces the hand-built infrastructure.

## Layout

- `steps/` — one buildable folder per step, each with `exercise/` and `solution/`
- `steps/common/` — the engine, the ring buffer, the fake host and the test signals, shared by every step
- `models/` — the exported seqsynth model (LibTorch `.pt`, ONNX)
- `setup/` — setup check
- `cmake/` — shared CMake helpers (backends, anira, RTSan)
- `slides/` — Reveal.js slides

## Setup

CMake ≥ 3.22, a C++17 compiler, and [Git LFS](https://git-lfs.com) for the
model file. The inference engines are downloaded as prebuilt binaries at
configure time; nothing else to install.

```bash
git lfs install && git lfs pull
```

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/bin/step01_solution
```

Build a single step with `-DWORKSHOP_STEP=01`. See
[models/README.md](models/README.md) for the model.

The steps so far:

1. [Minimal C++ inference](steps/01_minimal_inference/README.md) — load the model, run it, match the Python reference
2. [Model input size vs. host buffer size](steps/02_host_buffer_size/README.md) — ring buffers between the host's block size and the model's
3. [Benchmarking and the real-time budget](steps/03_benchmark/README.md) — how long a forward pass takes, and how long in the worst case

## License

Everything (code, slides, text): CC BY-NC 4.0, see `LICENSE`.
