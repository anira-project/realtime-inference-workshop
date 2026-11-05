# Step 4: Benchmarking and the real-time budget

**No exercise here** — this one is a demonstration. Build it, run it, read your numbers.

**Goal:** measure how long one forward pass takes on your machine — the mean and the slowest.

**Problem it reveals:** the first forward passes are slower than the rest, a warm-up. After it, the average is not what decides: every fourth callback has to fit a whole forward pass into its 11.6 ms, and a single slow pass is a dropout.

## The budget

A callback has to be finished before the host needs the next block:

```
budget = host block size / sample rate
```

At 44.1 kHz that is 11.6 ms for the host's 512 samples and 46.4 ms for a 2048-sample model block. The model needs a whole model block before it can run, but the deadline belongs to whichever callback it runs *in*: every fourth callback has to fit a whole forward pass into its 11.6 ms. The program prints both before it measures.

## What it does

[`solution/main.cpp`](solution/main.cpp) times the forward pass of the ONNX Runtime engine from step 2, with its state, using `std::chrono::steady_clock`. Only `engine.process(...)` stands between the two clock reads: that is what the worker in step 6 runs. The model is loaded before anything is timed.

It measures the way anira's benchmarks do: every repetition constructs a fresh engine (`k_repetitions = 5`), and every iteration times one single forward pass (`k_iterations = 200`). An average is the one number that cannot show a worst case, so every forward pass is timed on its own. The program shows the first forward passes of every engine side by side, the statistics after the warm-up, the slowest passes, and your worst case in samples. That reporting lives in [`common/checks/step04.h`](../common/checks/step04.h), so `main.cpp` shows only the measurement.

Build and run from the repository root:

```bash
cmake --preset release
cmake --build --preset release --target step04_solution
./build/bin/step04_solution
```

## Expected output

<!-- TODO: ONNX Runtime output from step04_solution on a quiet machine, with the numbers in the two paragraphs below. -->

It prints, in this order:

- the real-time thresholds: 46.44 ms for a 2048-sample model block, 11.61 ms for a 512-sample callback
- the first 5 forward passes of every repetition, side by side
- the statistics over all the forward passes: mean, median, min, p99, max
- the 5 slowest of all 1000 forward passes, with their repetition and pass
- your worst case, in milliseconds and in samples, rounded up to whole 512-sample callbacks

Your numbers will differ. The shape is what matters: **the first forward passes of every repetition are slower than the rest**, and it is not random, so a plugin has to make those calls before the audio starts. How much slower depends on the runtime: on LibTorch the first two passes are several times slower, because TorchScript optimises the model while it runs it. The slides show both.

The statistics and the last lines cover every forward pass, the slow first ones included: that is your worst case. Converted to samples at 44.1 kHz and rounded up to whole 512-sample callbacks, it is the `rounded_max_inference_time`: in step 6 the worker gets that much time, because the audio thread only picks results up once per callback. **Put your milliseconds into `k_max_inference_ms` in [`steps/common/inference_time.h`](../common/inference_time.h)**: steps 6, 7 and the plugin read it from there. Above it is the largest worst case that can work at all, one model block of time, 46.4 ms; your number has to be smaller, and if it is not, tell us.

Run it a few times. Sometimes a pass after the warm-up is slow too. That is not warm-up but the rest of the system getting in the way, which is why the worst case has to fit with room to spare: take the largest of a few runs, on a machine that is otherwise idle.

## If something looks wrong

- **Every number is far too large** — a Debug build measures something else entirely. Use the `release` preset.

## Slides

[`slides/04-benchmark.md`](../../slides/04-benchmark.md).

## What broke?

The measurement itself is fine. What it says is not: every fourth callback has to fit a whole forward pass into its 11.6 ms, and a single slow pass is a dropout.

And none of this is the whole story yet — we measured the forward pass on its own, on an ordinary thread. Next: what happens when that call sits in the audio callback.
