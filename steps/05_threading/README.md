# Step 5: Inference on a worker thread

**Goal:** get the engine off the audio thread, and keep the audio thread free of allocations and locks.

**What it answers:** step 4 showed that neither engine may run in the callback. This is where the inference goes instead.

## The shape

```
audio thread                     worker thread
------------                     -------------
process_block()                  while (running)
  push  ──→ m_input  ──────────→   pop 2048 samples
                                   engine.process()
  pop   ←── m_output ←──────────   push 2048 samples
```

Step 2's two ring buffers, now between two threads. [`ThreadSafeRingBuffer`](../common/helpers/threadsafe_ring_buffer.h) has the same functions as `RingBuffer` — `push`, `pop`, `available`, `space` — and is safe for one writer and one reader on different threads. Underneath is [moodycamel's](https://github.com/cameron314/readerwriterqueue) single-producer single-consumer circular buffer, used only through its `try_` functions: no lock, no wait, and after construction no allocation.

`available()` and `space()` seen from the other thread can only be too small, never too large, so checking first and then pushing or popping is safe.

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp) and fill in the three TODO banners.

1. **In `prepare()`**: size `m_input` for the blocks that can wait for the worker, and start the worker thread. Everything that allocates belongs here.
2. **In `process_block()`**: the audio thread. Step 2's `process_block`, without the engine: samples into `m_input`, samples out of `m_output` — and nothing that allocates, locks or waits.
3. **In `worker()`**: wait for a whole model block in `m_input`, run the engine, push the result into `m_output`. This side may sleep and allocate.

Each TODO says so while it is still open:

```
TODO 1: give m_input room for at least a host block and a model block
TODO 2: the audio thread never handed a block over.
TODO 3: the worker never picked a block up.
```

## What you get

```
  allocations on the audio thread 0
  the model produced           16384 of 16384 samples
  max abs diff vs. reference   4.92e-07

OK: inference ran on the worker thread, the audio thread only moved samples.
```

Two checks, both of which have to hold:

- **Zero allocations in the callback.** The program counts them with its own `operator new`, thread-local, so the worker thread's allocations — thousands per block, see step 4 — do not count.
- **The output still matches the reference**, to 4.92e-07, as in step 1.

Under RTSan the audio thread now reports nothing at all:

```bash
cmake -S . -B build-rtsan -DCMAKE_BUILD_TYPE=Release -DWORKSHOP_RTSAN=ON \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)"
cmake --build build-rtsan -j --target step05_solution
./build-rtsan/bin/step05_solution      # No report. The engine is on the other thread.
```

## The host runs in real time now

`run_host` paces the callbacks: one block of audio every `block_size / sample_rate`, 11.6 ms for 512 samples at 44.1 kHz. Up to now the loop ran as fast as it could, which no worker thread could ever keep up with — and with two threads, keeping up is the whole question.

## What the audio thread still does

Look at what is left in `process_block`: a ring buffer push and a ring buffer pop. No engine, no `new`, no mutex, no `join`.

And what it does when the worker is late: `m_output` has nothing ready and the host gets silence. That is a dropout, and it is a decision — the alternative, waiting for the worker, would be a missed deadline for the whole host.

## Two things that only show up under load

Both were found by running the plugin from step 8 through `pluginval`, and both
are in the solution:

- **A full ring buffer must drop, not stall.** If `m_input` has no room, the
  worker is behind. Waiting for room would stall the host, and pushing anyway
  overflows the buffer, so the block is dropped — a glitch, but a bounded one.
- **The worker must watch the stop flag while it waits.** Waiting for room in
  `m_output` without checking whether the processor is shutting down
  deadlocks `join()` as soon as the host stops calling the callback. A
  lock-free ring buffer does not save you from a deadlock at shutdown.

## Slides

[`slides/05-threading.md`](../../slides/05-threading.md).

## What's next

The audio thread is safe and the output is right, but it is late: the host gets silence until the worker returns the first block. How late exactly, and how do you tell the host — that is the next step.
