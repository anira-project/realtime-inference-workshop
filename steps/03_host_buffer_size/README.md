# Step 3: Model input size vs. host buffer size

**Goal:** feed a model that only accepts 2048 samples from a host that hands you 512 at a time.

**Problem it reveals:** the two sizes have nothing to do with each other. Something has to sit in between.

## Why this is not a corner case

The model's block size comes from the export. The host's comes from the audio device, the user's settings, and the sample rate. They match by accident, once. In this part of the workshop the host always hands over 512 samples: not the model's 2048, but a divisor of it, so four callbacks make one model block. Arbitrary host block sizes need more than this, and part 2 shows how.

## What you have

- [`common/helpers/ring_buffer.h`](../common/helpers/ring_buffer.h) — a ring buffer, written for you: `push`, `pop`, `available`, `space`. It throws on overflow and underflow, which is how you find out your capacity was wrong.
- `common/helpers/libtorch_engine.h` — the engine from step 1, unchanged.
- `common/helpers/support.h`, `common/assets/test_signal.h`, `common/assets/target_signal.h` — as before.
- [`common/checks/step03.h`](../common/checks/step03.h) — `check_todos`, called at the end of `process_block`: says which TODO is still missing. Not part of the exercise.

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp) and fill in `ProcessorExample`, at the three TODO banners. It has the shape a plugin has:

```cpp
processor.prepare(max_block_size);            // Before the audio starts: allocate
processor.process_block(samples, num_samples) // Per callback, in place
```

1. **Size the two ring buffers**, in `prepare()`. How much can be in flight at once, with a host block of 512 and a model block of 2048? Everything that allocates belongs here, not in the callback.
2. **The pump**, in `process_block()`. Push the host's samples into the input ring buffer, check whether a whole model block is waiting there, and if so pop it into `m_model_block`, run the model (given) and push the result into the output ring buffer. What is left over waits for the next call.
3. **Give the host its samples back**, in place: pop them from the output ring buffer, if enough are there. At the start the model has produced nothing, and the host gets silence (given).

Build and run from the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/bin/step03_exercise
```

## Expected output

```
OK: max abs diff 4.25e-07, within 0.0001 of the reference.
```

`main` plays the host: a loop that hands `process_block` 512 samples at a time, in place, like the loop in step 2. What came back has to be the model's stream from step 1, sample for sample, after a stretch of silence.

## If you're stuck

Each TODO says so when it is still missing:

- **`TODO 1: give the ring buffers a capacity in prepare()`** — `capacity` is still 0. Think about the worst moment on each side: the input just before a model block is taken out, the output just after one was put in.
- **`TODO 2: push the samples into m_input and m_output`** — both ring buffers are empty after a callback: the host's samples were not pushed into `m_input`, or the model's result was not pushed into `m_output`.
- **`TODO 2: run the model on the samples that arrived`** — the input is piling up: a whole model block is waiting, but `has_block` is still false or it is not popped from `m_input`.
- **`TODO 3: give the host its samples back`** — the output is piling up instead of being popped into the host's buffer.

The ring buffer complains too:

- **`ring buffer overflow: 512 samples pushed, room for …`** — the capacity from TODO 1 is still 0, or too small.
- **`ring buffer underflow`** — you are popping output that does not exist yet. Early on there is none; the condition in TODO 3 decides when the host gets silence instead.
- **`FAILED: max abs diff …`** — every TODO is filled in, but the host did not get the model's stream back: the samples are not the right ones, or not in the right place.

Compare with [`solution/main.cpp`](solution/main.cpp) when you want to.

## Slides

[`slides/03-host-buffer-size.md`](../../slides/03-host-buffer-size.md).

## What broke?

Nothing, but two things are now true that were not before. The host gets silence at the start: how long, and where it comes from, is the question after the solution. And every fourth callback now runs a whole forward pass. How long does that take, and what is the budget? That is step 4.
