# Real-Time Neural Inference Workshop

From an exported neural model to a real-time-safe audio plugin, in C++.

The workshop starts naive and runs into every problem on purpose: each step fixes the previous step's problem and exposes the next one, until [anira](https://github.com/anira-project/anira) replaces the hand-built infrastructure.

## Layout

- `steps/` — one buildable folder per step, each with `exercise/` and `solution/`; steps 4, 5 and 8 have no exercise, only `solution/`, and step 11 is a playground, `exercise/` only
- `steps/common/assets/` — the test signal and its target output, the example audio in `audio/`, and the models in `models/` (downloaded at configure time)
- `steps/common/helpers/` — the engines, the ring buffer, the fake host and the reporting, shared by every step
- `setup/` — setup check
- `cmake/` — shared CMake helpers (backends, models, anira, JUCE, exercises)
- `scripts/` — the slide build, the model export and test data, and `fill_exercise.py` for the exercise check
- `slides/` — Reveal.js slides

## Setup

CMake ≥ 3.22 and a C++17 compiler. Everything else is fetched at configure
time; nothing to install. **Configure once before the workshop**, at home:
about 500 MB, and the room's network is shared by everyone.

- the models, from this repository's release, into `steps/common/assets/models/`
- JUCE, ONNX Runtime and anira, with CMake's `FetchContent`, into the build folder

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release -j       # the libraries and every solution
./build/release/bin/step01_solution
```

Or with the preset, one step at a time:

```bash
cmake --preset release
cmake --build --preset release --target step01_exercise
./build/release/bin/step01_exercise
```

To step through a task in a debugger, use the debug preset. It builds into
`build/debug/`, next to the release build in `build/release/`:

```bash
cmake --preset debug
cmake --build --preset debug --target step01_exercise
./build/debug/bin/step01_exercise
```

Every configure links `build/compile_commands.json` to its own build folder, so
clangd (`.clangd`) follows the build configured last.

Timings, such as the benchmark in step 4, only mean something in the release
build.

## Exercises

Every blank in an exercise is a `?`, in the C++ and in the model files of
steps 9 and 10. An exercise builds once every `?` in its C++ is filled in, so
the default build leaves the exercises out: build one with
`--target stepNN_exercise` (`stepNN_exercise_check` and the plugin for steps 9
and 10).

That every exercise still fits its solution is checked by its filled-in copy:
[`scripts/fill_exercise.py`](scripts/fill_exercise.py) takes each `?` line
from the solution, and the copy has to build and pass like the solution. It
fails, naming the line, if an exercise was committed filled in or drifted away
from its solution. CI runs all of them; locally:

```bash
cmake --build --preset release --target step03_exercise_filled && ./build/release/bin/step03_exercise_filled
cmake --build --preset release --target step09_exercise_filled_check && ./build/release/bin/step09_exercise_filled_check
```

Filling needs Python 3; without it, CMake skips these targets.

Build a single step with `-DWORKSHOP_STEP=01`. See
[steps/common/assets/models/README.md](steps/common/assets/models/README.md) for
the model.

The steps so far:

1. [Minimal C++ inference](steps/01_minimal_inference/README.md) — load the model, run one block, match the Python reference
2. [Stateless and stateful](steps/02_state/README.md) — pass the state along, and run the whole signal
3. [Model input size vs. host buffer size](steps/03_host_buffer_size/README.md) — ring buffers between the host's block size and the model's
4. [Benchmarking and the real-time budget](steps/04_benchmark/README.md) — how long a forward pass takes, and how long in the worst case
5. [Real-time safety](steps/05_realtime_safety/README.md) — RTSan on both engines; demonstration, no exercise
6. [Inference on a worker thread](steps/06_threading/README.md) — lock-free ring buffers, and an audio thread that allocates nothing
7. [Latency and the dry/wet mix](steps/07_latency/README.md) — what to report to the host, and how to prove it
8. [The plugin](steps/08_juce_plugin/README.md) — all of it inside JUCE; solution only

Part 2, on anira (fetched and built by CMake, `-DWORKSHOP_ANIRA=OFF` to skip):

9. [The model with anira](steps/09_anira/README.md) — two JSON files and three calls replace steps 3, 6 and 7
10. [Opening the model](steps/10_split/README.md) — encoder and decoder as two handlers, the latent space in between
11. [The model as an instrument](steps/11_hands/README.md) — a camera and hand tracking drive the decoder; a JUCE app

## License

Everything (code, slides, text): CC BY-NC 4.0, see `LICENSE`.
