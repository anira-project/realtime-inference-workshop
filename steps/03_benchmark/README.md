# Step 3: Benchmarking and the real-time budget

**Goal:** measure how long one forward pass takes — the mean and the slowest.

**Problem it reveals:** the mean is comfortable and the worst case is not, and only the worst case decides whether you get a dropout.

## The budget

A callback has to be finished before the host needs the next block:

```
budget = host block size / sample rate
```

At 44.1 kHz that is 1.45 ms for 64 samples, 11.6 ms for 512, 46.4 ms for 2048. The model needs a whole 2048-sample block before it can run, but the deadline belongs to whichever callback it runs *in* — so the smaller the host block, the worse the mismatch. The program prints the table before it measures.

## What you have

- [Google Benchmark](https://github.com/google/benchmark), fetched by CMake — the same library anira benchmarks with.
- `common/helpers/libtorch_engine.h` and `common/assets/test_signal.h`, as before.

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp) and fill in the two TODO banners.

1. **Call the model inside the timed loop.** Only what stands in that loop is measured.
2. **Fix the registration.** Left alone, Google Benchmark runs the body as often as it likes and reports the average — the one number that cannot show a worst case. One call per repetition, `k_repetitions` of them. `slowest()`, `percentile()` and the statistics over them are given.

Each TODO says so while it is still open, in the order above.

Build and run from the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/bin/step03_exercise
```

## Expected output

On an M-series laptop, Release build:

```
forward_pass/iterations:1/repeats:200_mean         7.86 ms
forward_pass/iterations:1/repeats:200_median       7.37 ms
forward_pass/iterations:1/repeats:200_stddev       4.16 ms
forward_pass/iterations:1/repeats:200_min          7.12 ms
forward_pass/iterations:1/repeats:200_p95          8.15 ms
forward_pass/iterations:1/repeats:200_p99          10.2 ms
forward_pass/iterations:1/repeats:200_max          56.0 ms
```

Your numbers will differ. The shape is what matters: **the max is close to an order of magnitude above the median**, and it is not random. It is the very first call: TorchScript optimises the model while it runs it, and the second call is still slow. After that the calls stay around 7.5 ms, with a worst case of about 10-12 ms. The first call is above the 46.4 ms budget of a 2048-sample block, so a plugin has to make those calls before the audio starts.

Run it a few times. The mean, median and min barely move, and the max stays large, because the first call is slow every time.

## If you're stuck

- **`ERROR OCCURRED: 'TODO 1: call the model inside the timed loop'`** — the loop body is still empty.
- **`ERROR OCCURRED: 'TODO 2: time one single call per repetition'`** — the registration still lets Google Benchmark choose, so every repetition is already an average.
- **Every number is suspiciously small** — a Debug build measures something else entirely. Use `-DCMAKE_BUILD_TYPE=Release`.

Compare with [`solution/main.cpp`](solution/main.cpp) when you want to.

## Slides

[`slides/03-benchmark.md`](../../slides/03-benchmark.md).

## What broke?

The measurement itself is fine. What it says is not: the mean fits in the budget, the worst case does not, and the worst case is the only number a real-time deadline cares about.

And none of this is the whole story yet — we measured the forward pass on its own, on an ordinary thread. Next: what happens when that call sits in the audio callback.
