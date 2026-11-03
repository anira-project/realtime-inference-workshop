# Step 3: Benchmarking and the real-time budget

**No exercise here** — this one is a demonstration. Build it, run it, read your numbers.

**Goal:** measure how long one forward pass takes on your machine — the mean and the slowest.

**Problem it reveals:** the first calls are a warm-up, far slower than the rest. Once warmed up, the model fits a model block, but not a small host callback.

## The budget

A callback has to be finished before the host needs the next block:

```
budget = host block size / sample rate
```

At 44.1 kHz that is 1.45 ms for 64 samples, 11.6 ms for 512, 46.4 ms for 2048. The model needs a whole 2048-sample block before it can run, but the deadline belongs to whichever callback it runs *in* — so the smaller the host block, the worse the mismatch. The program prints the table before it measures.

## What it does

[`solution/main.cpp`](solution/main.cpp) runs the model through [Google Benchmark](https://github.com/google/benchmark), fetched by CMake — the same library anira benchmarks with. Only what stands in the timed loop is measured, and the model is loaded before anything is timed.

Left alone, Google Benchmark runs the body as often as it likes and reports the average — the one number that cannot show a worst case. So every repetition times a single call (`Iterations(1)`), 200 of them (`Repetitions(k_repetitions)`), and the program keeps the time of each call, so that below the usual table it can draw the first calls and list the slowest ones.

Build and run from the repository root:

```bash
cmake --preset release
cmake --build --preset release --target step03_solution
./build/bin/step03_solution
```

## Expected output

On an M-series laptop, Release build:

```
forward_pass/iterations:1/repeats:200_mean         9.11 ms         8.32 ms          200
forward_pass/iterations:1/repeats:200_median       8.08 ms         7.66 ms          200
forward_pass/iterations:1/repeats:200_stddev       8.08 ms         4.90 ms          200
forward_pass/iterations:1/repeats:200_cv          88.72 %         58.87 %           200
forward_pass/iterations:1/repeats:200_min          7.52 ms         7.17 ms          200
forward_pass/iterations:1/repeats:200_p95          9.94 ms         9.16 ms          200
forward_pass/iterations:1/repeats:200_p99          12.1 ms         10.9 ms          200
forward_pass/iterations:1/repeats:200_max          95.7 ms         64.0 ms          200

The first 10 of 200 calls:
  call  1  ##################################        81.67 ms
  call  2  ########################################  95.66 ms
  call  3  ###                                        8.31 ms
  call  4  ####                                       8.67 ms
  ...

The 5 slowest calls:
  call   2   95.66 ms
  call   1   81.67 ms
  call 177   12.05 ms
  call  48   11.93 ms
  call  50   11.22 ms
```

Your numbers will differ. The shape is what matters: **the first two calls are far slower than the rest**, and it is not random. TorchScript optimises the model while it runs it, so a plugin has to make those calls before the audio starts.

The max in the table is one of those first calls. To find your worst case after the warm-up, look at the list of the slowest calls: it is the first entry that is not call 1 or 2, here 12.05 ms.

Run it a few times. The first calls are slow every time. Sometimes a call after the warm-up is slow too: one run here had 43.38 ms, a factor of 0.93 against a model block. That is not warm-up but the rest of the system getting in the way, which is why the worst case has to fit with room to spare.

Google Benchmark may print a warning about the CPU frequency or thread affinity. It does not affect the measurement.

## If something looks wrong

- **Every number is far too large** — a Debug build measures something else entirely. Use the `release` preset.

## Slides

[`slides/03-benchmark.md`](../../slides/03-benchmark.md).

## What broke?

The measurement itself is fine. What it says is not: warmed up, the model fits a model block, but a 64-sample callback has 1.45 ms and the call takes several times that.

And none of this is the whole story yet — we measured the forward pass on its own, on an ordinary thread. Next: what happens when that call sits in the audio callback.
