# Step 2: Model input size vs. host buffer size

**Goal:** feed a model that only accepts 2048 samples from a host that hands you whatever it likes.

**Problem it reveals:** the two sizes have nothing to do with each other, and they never will. Something has to sit in between.

## Why this is not a corner case

The model's block size comes from the export. The host's comes from the audio device, the user's settings, and the sample rate — 64, 128, 480, 512, and it can change while you are running. They match by accident, once.

## What you have

- [`common/ring_buffer.h`](../common/ring_buffer.h) — a ring buffer, written for you: `push`, `pop`, `available`, `space`. It throws on overflow and underflow, which is how you find out your capacity was wrong.
- [`common/host.h`](../common/host.h) — a fake host: it calls your `process(samples, num_samples)` with one block at a time, at a size you choose. No audio device, so it runs anywhere.
- `common/libtorch_engine.h` — the engine from step 1, unchanged.
- `common/support.h`, `common/test_signal.h`, `common/target_signal.h` — as before.

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp) and fill in `ProcessorExample`, at the three TODO banners. It has the shape a plugin has:

```cpp
processor.prepare(max_block_size);            // Before the audio starts: allocate
processor.process_block(samples, num_samples) // Per callback, in place
```

1. **Size the two ring buffers**, in `prepare()`. How much can be in flight at once, given a host block of N and a model block of 2048? Everything that allocates belongs here, not in the callback.
2. **The pump**, in `process_block()`. Take the host's samples in, and run the model whenever a whole block has arrived. What is left over waits for the next call.
3. **Give the host its samples back**, in place. At the start the model has produced nothing — so what goes out?

Build and run from the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/bin/step02_exercise
```

## Expected output

```
  host block 64            max abs diff 2.21e-06   OK
  host block 128           max abs diff 2.21e-06   OK
  host block 480           max abs diff 2.21e-06   OK
  host block 512           max abs diff 2.21e-06   OK
  host block 1024          max abs diff 2.21e-06   OK
  host block 2048          max abs diff 2.21e-06   OK

OK: the model saw the same stream at every host block size.
```

The check is on **what the model produced**, not on what the host received: the same stream as in step 1, regardless of how the host chopped up the input. 480 does not divide the signal, so that run simply stops a bit earlier.

## If you're stuck

- **`produced 0 samples, expected 6144`** — the model is never run; TODO 2 is still empty.
- **`ring buffer overflow: 64 samples pushed, room for 0`** — the capacity from TODO 1 is too small. Think about the worst moment: the host has just written a full block and the model has not taken anything out yet.
- **`ring buffer underflow`** — you are popping output that does not exist yet. Early on there is none; TODO 3 decides what the host gets instead.
- **`produced 4096 samples, expected 6144`** — samples are being dropped. Everything the host gives you has to go in, and only whole model blocks come out.
- **It passes at 2048 and fails everywhere else** — the pump only works when the sizes happen to match: probably one model block per callback, rather than as many as are ready.

Compare with [`solution/main.cpp`](solution/main.cpp) when you want to.

## Slides

[`slides/02-host-buffer-size.md`](../../slides/02-host-buffer-size.md).

## What broke?

Nothing, but two things are now true that were not before. The host gets silence at the start, because the model needs 2048 samples before it can say anything — that is latency, and it is a later step. And every callback now either does nothing or runs a whole forward pass. How long does that take, and what is the budget? That is step 3.
