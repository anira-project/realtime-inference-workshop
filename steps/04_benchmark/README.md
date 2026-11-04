# Step 4: Benchmarking and the real-time budget

**No exercise here** — this one is a demonstration. Build it, run it, read your numbers.

**Goal:** measure how long one forward pass takes on your machine — the mean and the slowest.

**Problem it reveals:** the first calls are a warm-up, far slower than the rest. Once warmed up, the model fits even a 512-sample host callback on average, but not with room to spare, and not every call.

## The budget

A callback has to be finished before the host needs the next block:

```
budget = host block size / sample rate
```

At 44.1 kHz that is 11.6 ms for the host's 512 samples and 46.4 ms for a 2048-sample model block. The model needs a whole model block before it can run, but the deadline belongs to whichever callback it runs *in*: every fourth callback has to fit a whole forward pass into its 11.6 ms. The program prints both before it measures.

## What it does

[`solution/main.cpp`](solution/main.cpp) runs the model through [Google Benchmark](https://github.com/google/benchmark), fetched by CMake — the same library anira benchmarks with. Only what stands in the timed loop is measured, and the model is loaded before anything is timed.

Left alone, Google Benchmark runs the body as often as it likes and reports the average — the one number that cannot show a worst case. So every repetition times a single call (`Iterations(1)`), 200 of them (`Repetitions(k_repetitions)`), and the program keeps the time of each call, so that below the usual table it can draw the first calls and list the slowest ones.

Build and run from the repository root:

```bash
cmake --preset release
cmake --build --preset release --target step04_solution
./build/bin/step04_solution
```

## Expected output

On an M-series laptop, Release build:

```
forward_pass_onnx_mean 13.2 ms 13.2 ms 200
forward_pass_onnx_min 12.8 ms 12.7 ms 200
forward_pass_onnx_p95 13.6 ms 13.5 ms 200
forward_pass_onnx_p99 14.5 ms 14.3 ms 200
forward_pass_onnx_max 19.0 ms 18.1 ms 200
forward_pass_libtorch_mean 8.51 ms 7.89 ms 200
forward_pass_libtorch_min 7.45 ms 7.09 ms 200
forward_pass_libtorch_p95 8.65 ms 8.13 ms 200
forward_pass_libtorch_p99 22.7 ms 12.4 ms 200
forward_pass_libtorch_max 79.6 ms 60.3 ms 200

ONNX Runtime, the first 10 of 200 calls:
  call  1  ########################################  18.97 ms
  call  2  ############################              13.24 ms
  call  3  ###########################               12.91 ms
  ...

LibTorch, the first 10 of 200 calls:
  call  1  ########################################  79.62 ms
  call  2  ###################                       38.42 ms
  call  3  ####                                       7.84 ms
  ...

The two engines, after the warm-up:
               fastest   slowest
  ONNX Runtime  12.77 ms  15.30 ms
  LibTorch       7.45 ms  22.65 ms

Your worst case on ONNX Runtime after the warm-up (calls 3 to 200):
  max inference time            15.30 ms = 675 samples
  rounded_max_inference_time   1024 samples (whole 512-sample callbacks)

Put 15.30 into k_max_inference_ms in steps 6 and 7.
```

Your numbers will differ. The shape is what matters: **the first calls are slower than the rest**, and it is not random. On LibTorch they are far slower, because TorchScript optimises the model while it runs it. A plugin has to make those calls before the audio starts.

The two engines run the same model but behave differently. Here ONNX Runtime has a short warm-up and a slower typical call, while LibTorch has a long warm-up and a faster typical call. The slowest call changes from run to run on both, and gets much worse when the machine is busy.

The last lines leave the first calls out: that is your worst case on ONNX Runtime after the warm-up, here 15.30 ms, or 675 samples at 44.1 kHz. Rounded up to whole 512-sample callbacks it is 1024 samples, the `rounded_max_inference_time`: in step 6 the worker gets that much time, because the audio thread only picks results up once per callback. **Write your milliseconds down**, steps 6 and 7 ask for them as `k_max_inference_ms`.

Run it a few times. The first calls are slow every time. Sometimes a call after the warm-up is slow too: one LibTorch run here had 43.38 ms, a factor of 0.93 against a model block and 3.7 against a 512-sample callback. That is not warm-up but the rest of the system getting in the way, which is why the worst case has to fit with room to spare: take the largest of a few runs.

Google Benchmark may print a warning about the CPU frequency or thread affinity. It does not affect the measurement.

## If something looks wrong

- **Every number is far too large** — a Debug build measures something else entirely. Use the `release` preset.

## Slides

[`slides/04-benchmark.md`](../../slides/04-benchmark.md).

## What broke?

The measurement itself is fine. What it says is not: warmed up, the call fits a model block easily, but on ONNX Runtime it does not even fit the 11.6 ms of a 512-sample callback on average, and every fourth callback has to run it.

And none of this is the whole story yet — we measured the forward pass on its own, on an ordinary thread. Next: what happens when that call sits in the audio callback.
