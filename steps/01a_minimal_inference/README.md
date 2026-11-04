# Step 1a: Minimal C++ inference

**Goal:** run one block through the model in C++ and prove the output is the same as the reference.

## What you have

- `models/model_forward.onnx` — the model from the slides (magnetic resonator piano) as ONNX: graph and weights in one file. 2048 samples per call, 44.1 kHz, one channel.
- [`common/helpers/onnx_engine.h`](../common/helpers/onnx_engine.h) — the engine, written for you. Read it first.
- [`common/assets/test_signal.h`](../common/assets/test_signal.h) — `k_input_signal`: a 220 Hz sine, 8 blocks of 2048 samples.
- `common/assets/target_signal.h` — `k_target_output_signal`: what the model produced for it in Python.
- `common/helpers/support.h` — the comparison and the report.

Both signals are compiled in, so there are no files to read and no audio format to parse.

The engine loads the model and processes the data:

```cpp
OnnxEngine engine(model_path);            // Loads the model, throws if it cannot

engine.process({audio}, {num_samples});   // Overwrites the block in audio
```

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp) and fill in the three TODO banners.

1. **Set the path to the model.**
2. **Pick the size you process** (`process_size`). It starts unset, and the program says so. Choose a number, run it, and see whether the model agrees with you.
3. **Run the first block** through the engine.

Build and run from the repository root:

```bash
cmake --preset release
cmake --build --preset release --target step01a_exercise
./build/bin/step01a_exercise
```

## Expected output

```
OK: max abs diff 2.33e-10, within 1e-06 of the reference.
```

## If you're stuck

- **`TODO 1: set the path to the model first`** — `model_path` is empty or wrong.
- **`TODO 2: pick a size to process first.`** — `process_size` is still 0.
- **The program stops with `the model takes exactly 2048 samples per call, got M`** — the size you picked is not the model's block: `k_model.m_input_size`.
- **`FAILED: max abs diff 0.5`** — the engine is never used, so `audio` still holds the input.

Compare with [`solution/main.cpp`](solution/main.cpp) when you want to.

## Slides

[`slides/01a-minimal-inference.md`](../../slides/01a-minimal-inference.md), or the deployed deck.

## What's next?

One block matches. Step 1b runs the whole signal.
