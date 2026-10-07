# Step 6: Inference on a worker thread

**Goal:** get the engine off the audio thread, and keep the audio thread free of allocations and locks.

**What it answers:** step 5 showed that neither engine may run in the callback. This is where the inference goes instead.

## The shape

```
audio thread                     worker thread
------------                     -------------
process_block()                  while (running)
  ring buffer in                   try_dequeue  ←── to_worker queue
  try_enqueue  ──→ to_worker       engine.process()
  try_dequeue  ←── from_worker     try_enqueue  ──→ from_worker queue
  ring buffer out
```

Two [moodycamel `ReaderWriterQueue`s](https://github.com/cameron314/readerwriterqueue), one per direction. Single producer, single consumer, lock-free, and `try_enqueue` never grows the queue — it fails instead, which is exactly what the audio thread needs.

What crosses the queue is a `ModelBlock`: a fixed `std::array<float, 2048>` by value. Copying 8 KB is a `memcpy`; passing a pointer would mean agreeing on who frees it.

## What you get

```
  allocations on the audio thread 0
  the model produced           6144 of 6144 samples
  max abs diff vs. reference   2.21e-06

OK: inference ran on the worker thread, the audio thread only moved samples.
```

Two checks, both of which have to hold:

- **Zero allocations in the callback.** The program counts them with its own `operator new`, thread-local, so the worker thread's allocations — tens of thousands per block, see step 5 — do not count.
- **The output still matches the reference**, to 2.21e-06, as in step 1.

Under RTSan the audio thread now reports nothing at all:

```bash
cmake -S . -B build-rtsan -DCMAKE_BUILD_TYPE=Release -DWORKSHOP_RTSAN=ON \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)"
cmake --build build-rtsan -j --target step06_solution
./build-rtsan/bin/step06_solution      # No report. The engine is on the other thread.
```

## The host runs in real time now

`run_host` paces the callbacks: one block of audio every `block_size / sample_rate`, 10.7 ms for 512 samples at 48 kHz. Up to now the loop ran as fast as it could, which no worker thread could ever keep up with — and with two threads, keeping up is the whole question.

## What the audio thread still does

Look at what is left in `process_block`: a ring buffer push, a `try_enqueue`, a `try_dequeue`, a ring buffer pop. No engine, no `new`, no mutex, no `join`.

And what it does when the worker is late: `try_dequeue` returns nothing and the host gets silence. That is a dropout, and it is a decision — the alternative, waiting for the worker, would be a missed deadline for the whole host.

## Slides

[`slides/06-threading.md`](../../slides/06-threading.md).

## What's next

The audio thread is safe and the output is right, but it is late: the host gets silence until the worker returns the first block. How late exactly, and how do you tell the host — that is the next step.
