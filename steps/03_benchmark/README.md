# Step 3: Benchmarking and the real-time budget

**Goal:** measure how long one forward pass takes — mean, p95, p99, and the worst one.

**Problem it reveals:** the mean is comfortable and the worst case is not, and only the worst case decides whether you get a dropout.

## The budget

A callback has to be finished before the host needs the next block:

```
budget = host block size / sample rate
```

At 48 kHz that is 1.33 ms for 64 samples, 10.7 ms for 512, 42.7 ms for 2048. The model needs a whole 2048-sample block before it can run, but the deadline belongs to whichever callback it runs *in* — so the smaller the host block, the worse the mismatch. The program prints the table before it measures.

## What you have

- [Google Benchmark](https://github.com/google/benchmark), fetched by CMake — the same library anira benchmarks with.
- `common/libtorch_engine.h` and `common/test_signal.h`, as before.

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp) and fill in the three TODO banners.

1. **Call the model inside the timed loop.** Only what stands in that loop is measured.
2. **Implement `percentile()`.** Google Benchmark hands you one time per repetition; sort a copy and index into it.
3. **Fix the registration.** Left alone, Google Benchmark runs the body as often as it likes and reports the average — the one number that cannot show a worst case. One call per repetition, `k_repetitions` of them.

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
forward_pass/iterations:1/repeats:200_mean         5.13 ms
forward_pass/iterations:1/repeats:200_median       4.57 ms
forward_pass/iterations:1/repeats:200_stddev       5.83 ms
forward_pass/iterations:1/repeats:200_p95          5.21 ms
forward_pass/iterations:1/repeats:200_p99          5.36 ms
forward_pass/iterations:1/repeats:200_max          84.5 ms
```

Your numbers will differ. The shape is what matters: **the max is an order of magnitude above the median.** One call in two hundred took 84 ms, against a median of 4.6 ms — and with 2048-sample blocks the budget is 42.7 ms, so that one call misses the deadline twice over.

Run it a few times. The mean and median barely move; the max moves a lot.

## If you're stuck

- **`ERROR OCCURRED: 'TODO 1: call the model inside the timed loop'`** — the loop body is still empty.
- **`ERROR OCCURRED: 'TODO 3: time one single call per repetition'`** — the registration still lets Google Benchmark choose, so every repetition is already an average.
- **`TODO 2: implement percentile(), it still returns 0`**, and `p95`/`p99` read `0.000 ms` next to a mean of several milliseconds.
- **Every number is suspiciously small** — a Debug build measures something else entirely. Use `-DCMAKE_BUILD_TYPE=Release`.

Compare with [`solution/main.cpp`](solution/main.cpp) when you want to.

## Slides

[`slides/03-benchmark.md`](../../slides/03-benchmark.md).

## What broke?

The measurement itself is fine. What it says is not: the mean fits in the budget, the worst case does not, and the worst case is the only number a real-time deadline cares about.

And none of this is the whole story yet — we measured the forward pass on its own, on an ordinary thread. Next: what happens when that call sits in the audio callback.
