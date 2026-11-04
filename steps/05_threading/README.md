# Step 5: Inference on a worker thread

**Goal:** get the engine off the audio thread, keep the audio thread free of allocations and locks, and keep the latency fixed.

**What it answers:** step 4 showed that neither engine may run in the callback. This is where the inference goes instead.

## The shape

```
audio thread                     worker thread
------------                     -------------
process_block()                  while (running)
  try_push ──→ m_input  ───────→   pop 2048 samples
                                   engine.process()
  try_pop  ←── m_output ←───────   try_push 2048 samples
               (starts with the prefill)
```

Step 2's two ring buffers, now between two threads. [`ThreadSafeRingBuffer`](../common/helpers/threadsafe_ring_buffer.h) is safe for one writer and one reader on different threads. Underneath is [moodycamel's](https://github.com/cameron314/readerwriterqueue) single-producer single-consumer circular buffer, used only through its `try_` functions: no lock, no wait, and after construction no allocation. `try_push` and `try_pop` move one sample and say whether it worked; `available` tells the worker whether a whole block is waiting.

## The prefill

The worker needs time, and the host must always find something to take. So `m_output` starts with silence, as many samples as the host is late:

```
prefill = (2048 − 512)                  buffer adaptation, as in step 2
        + rounded_max_inference_time    the worker's time
```

`rounded_max_inference_time` is your worst case from step 3 in samples, rounded up to whole 512-sample callbacks: the audio thread only picks results up once per callback. Put your milliseconds from step 3 into `k_max_inference_ms`.

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp) and fill in the three TODO banners.

1. **In `prepare()`**: work out `rounded_max_inference_time`, the prefill and the size of `m_output` (the worker's time plus one model block), and start the worker thread. Everything that allocates belongs here; so does the warm-up (given).
2. **In `process_block()`**: the audio thread. Step 2's `process_block`, without the engine, one sample at a time:
   - push into `m_input`. If it is full, the worker is a whole block behind: count the rest in `m_unpushed`, the host gets that many zeros instead (given), and nothing is taken out of `m_output` for them;
   - pop from `m_output`. If a sample is not there yet, or something is still owed, the worker is late: the rest of the callback goes out as zeros, all of it owed, `m_debt`. Not just that one sample: the worker may be pushing right now, and the next one would land one place too early;
   - pay the debt first: an owed sample came too late, its place already went out as a zero, so it is taken out and thrown away.
3. **In `worker()`**: check for a whole model block in `m_input`, run the engine, push the result into `m_output`. No sleep: check again right away. If `m_output` is full, try again until there is room, but keep watching `m_running`.

Each TODO says so while it is still open:

```
TODO 1: put your worst case from step 3 into k_max_inference_ms
TODO 1: round k_max_inference_ms up to whole callbacks, in samples
TODO 1: start the worker thread.
TODO 2 or 3: the host only got silence. ...
```

## What you get

```
  prefill                      2048 samples (46.4 ms)
  allocations on the audio thread 0
OK: max abs diff 3.28e-07, within 0.0001 of the reference.
OK: inference ran on the worker thread, the host got the model's stream back
2048 samples late.
```

The prefill is 2048 for a worst case of up to 11.6 ms; yours may be more. The check listens like the host:

- **Zero allocations in the callback.** The program counts them with its own `operator new`, thread-local, so the worker thread's allocations — thousands per block, see step 4 — do not count.
- **The host hears the prefill in silence, then the model's stream**, to 3.28e-07 of the reference. A callback with zeros where the reference is not silent is a dropout: the worker was late. More than a quarter of them fail the run, and so does any sample that is neither the reference nor a zero.

Under RTSan the audio thread now reports nothing at all:

```bash
cmake -S . -B build-rtsan -DCMAKE_BUILD_TYPE=Release -DWORKSHOP_RTSAN=ON \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)"
cmake --build build-rtsan -j --target step05_solution
./build-rtsan/bin/step05_solution      # No report. The engine is on the other thread.
```

## The host runs in real time now

Step 2's host loop is now [`run_host`](../common/helpers/host.h), and it paces the callbacks: one block of audio every `block_size / sample_rate`, 11.6 ms for 512 samples at 44.1 kHz. Up to now the loop ran as fast as it could, which no worker thread could ever keep up with — and with two threads, keeping up is the whole question.

## What the audio thread still does

Look at what is left in `process_block`: pushing and popping samples. No engine, no `new`, no mutex, no `join`, no waiting.

And what it does when the worker is late: it never waits. The host gets zeros, a dropout, and the samples that came too late are thrown away when they arrive, so the stream stays exactly the prefill late. Without that, every late block would make the output a little later for good.

## Two things that only show up under load

- **A full ring buffer must not stall the audio thread.** If `m_input` has no room, the worker is a whole block behind. Waiting would stall the host, so the samples that do not fit become zeros on the way out. They never reach the model, which is a glitch, but a bounded one.
- **The worker must watch the stop flag while it waits.** Waiting for room in `m_output` without checking whether the processor is shutting down deadlocks `join()` as soon as the host stops calling the callback. A lock-free ring buffer does not save you from a deadlock at shutdown.

## Slides

[`slides/05-threading.md`](../../slides/05-threading.md).

## What's next

The audio thread is safe and the host hears the model's stream, the prefill late. But the model itself answers late too, and the host has to be told one number for all of it, with a dry signal that lines up: that is the next step.
