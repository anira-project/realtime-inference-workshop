# Step 4: Minimal C++ inference

**Goal:** run the model in C++ and prove the output is the same as the reference.

**Problem it reveals:** the result is correct. Now: how fast, and how fast in the *worst case*?

## What you have

- `models/forward_stateful.pt` — the model as TorchScript: graph and weights in one file. Audio in, audio out, 2048 samples per call, 48 kHz, one channel. The state lives inside the model, so each call continues where the last one ended.
- [`helpers/libtorch_engine.h`](helpers/libtorch_engine.h) — the engine, written for you. Read it first: it is the whole of LibTorch you need.
- [`helpers/test_signal.h`](helpers/test_signal.h) — `k_input_signal`: a 220 Hz sine, 3 blocks of 2048 samples.
- `helpers/target_signal.h` — `k_target_output_signal`: what the model produced for it in Python.
- `helpers/support.h` — the comparison and the report.

Both signals are compiled in, so there are no files to read and no audio format to parse. That comes later.

The engine is three methods:

```cpp
LibTorchEngine engine(model_path);        // Loads the model, throws if it cannot
engine.process(samples, num_samples);     // One block, in place
engine.reset();                           // Clears the state for a new stream, like prepare()
```

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp) and fill in the three TODO banners.

1. **Create the engine.** The `try`/`catch` around it is given. Note where it sits — outside the loop — and why that matters: loading is slow, and the model carries its state from block to block.
2. **Pick the size you process in** (`process_size`). It starts unset, and the program says so. Choose a number, run it, and see whether the model agrees with you. Whole blocks only: a size that does not divide the signal leaves the remainder unprocessed.
3. **Process each block**, in place.

Build and run from the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/bin/step04_exercise
```

## Expected output

```
OK: max abs diff 2.21e-06, within 0.0001 of the reference.
```

Not bit-identical, and that is the point: the reference ran in a different runtime (ONNX Runtime, in Python). Agreeing to about 1e-6 is what "the same model" means across engines.

The first block is nearly silent, because the model's latency has not been filled yet. It still has to match.

## If you're stuck

- **`TODO 2: pick a size to process in first.`** — `process_size` is still 0.
- **`process() failed: RuntimeError: The size of tensor a (6) must match ...`** — the size you picked is smaller than the model's block. The export fixed that size: `k_model.m_input_size`, 2048 samples, which is what the reference output was produced with. Larger sizes may go through, but nothing promises the model still lines up with the reference.
- **`process() failed: the model returned N samples for M in`** — the same thing from the other side: the model emits whole blocks, so it handed back a different number of samples than it was given.
- **`FAILED: max abs diff 0.904`** — the engine is never used, so `output` still holds the input.
- **`cannot load .../forward_stateful.pt`** — the model is not in the repo, see [models/README.md](../../models/README.md). Point the build at your copy with `-DWORKSHOP_MODEL=/path/to/forward_stateful.pt`.
- **It is slow and most blocks are wrong** — the engine is being built inside the loop. Every new model starts from a cleared state, so every block is treated as the first one, and a 65 MB file is loaded each time.

Compare with [`solution/main.cpp`](solution/main.cpp) when you want to.

## Slides

[`slides/04-minimal-inference.md`](../../slides/04-minimal-inference.md), or the deployed deck at `/04-minimal-inference.html`.

## What broke?

Nothing — the numbers are right. But a forward pass took some amount of time you have not measured, and at 48 kHz with 2048-sample blocks you have 42.7 ms per block. Not on average: every single time. That is step 5.
