# Real-Time Neural Inference Workshop

From an exported neural model to a real-time-safe audio plugin, in C++.

The workshop starts naive and runs into every problem on purpose: each step fixes the previous step's problem and exposes the next one, until [anira](https://github.com/anira-project/anira) replaces the hand-built infrastructure.

## Layout

- `steps/` — one buildable folder per step, each with `exercise/` and `solution/`
- `models/` — the exported seqsynth model (LibTorch `.pt`, ONNX) and reference output
- `setup/` — setup check
- `cmake/` — shared CMake helpers (backends, anira, RTSan)
- `slides/` — Reveal.js slides

## Setup

TODO

## License

Everything (code, slides, text): CC BY-NC 4.0, see `LICENSE`.
