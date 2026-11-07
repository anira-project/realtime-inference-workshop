# Step 1: Minimal C++ inference

**Goal:** run one block through the model in C++ and prove the output is the same as the reference.

## What you have

- `steps/common/assets/models/rave/model_forward.onnx` — the model from the slides (magnetic resonator piano) as ONNX: graph and weights in one file. 2048 samples per call, 44.1 kHz, one channel.
- [`common/helpers/onnx_engine.h`](../common/helpers/onnx_engine.h) — the engine, written for you. Read it first.
- [`common/assets/test_signal.h`](../common/assets/test_signal.h) — `k_input_signal`: a 220 Hz sine, 8 blocks of 2048 samples.
- `common/assets/target_signal.h` — `k_target_output_signal`: what the model produced for it in Python.
- [`common/checks/step01.h`](../common/checks/step01.h) — says which TODO is off, and compares the first block with the reference. Not part of the exercise.

Both signals are compiled in, so there are no files to read and no audio format to parse.

The engine loads the model and processes the data:

```cpp
OnnxEngine engine;                        // Nothing loaded yet
engine.load(path);                        // Loads the model, throws if it cannot

engine.process(audio, num_samples);       // Overwrites the block in place
```

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp) and fill in the two TODO banners.

1. **Load the model**: `load` takes the path to the `.onnx` file.
2. **Run the first block** through the engine: where it starts, and how many samples. Choose a number, run it, and see whether the model agrees with you.

Build and run from the repository root:

```bash
cmake --preset release
cmake --build --preset release --target step01_exercise
./build/release/bin/step01_exercise
```

## Expected output

```
  PASS  the output matches the reference: max abs diff 2.33e-10, tolerance 1e-06

==============================================================
  STEP 1 SOLVED
==============================================================
```

Anything else ends in `STEP 1 NOT SOLVED`, after a `FAIL` line that says what is off.

## If you're stuck

The exercise builds once every `?` is filled in. Then:

- **`cannot load …`** — the path in TODO 1 is not the model: `k_model.m_path`.
- **`the model takes exactly 2048 samples per call, got M`** — the size you picked is not the model's block: `k_model.m_input_size`.
- **`TODO 2: run the first block through the engine`** — `buffer` still holds the input: the engine did not process it in place.

Compare with [`solution/main.cpp`](solution/main.cpp) when you want to.

## Slides

[`slides/01-minimal-inference.md`](../../slides/01-minimal-inference.md), or the deployed deck.

## What's next?

One block matches. Step 2 runs the whole signal.
