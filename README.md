# Real-Time Neural Inference Workshop

From an exported neural model to a real-time-safe audio plugin, in C++.

The workshop starts naive and runs into every problem on purpose: each step fixes the previous step's problem and exposes the next one, until [anira](https://github.com/anira-project/anira) replaces the hand-built infrastructure.

## Layout

- `steps/` — one buildable folder per step, each with `exercise/` and `solution/`
- `models/` — the exported seqsynth model (LibTorch `.pt`, ONNX)
- `setup/` — setup check
- `cmake/` — shared CMake helpers (backends, anira, RTSan)
- `slides/` — Reveal.js slides

## Setup

CMake ≥ 3.22 and a C++17 compiler. The inference engines are downloaded as
prebuilt binaries at configure time; nothing else to install.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/bin/step04_solution
```

Build a single step with `-DWORKSHOP_STEP=04`. The model file is not in the
repo, see [models/README.md](models/README.md).

Start with [steps/04_minimal_inference](steps/04_minimal_inference/README.md).

## License

Everything (code, slides, text): CC BY-NC 4.0, see `LICENSE`.
