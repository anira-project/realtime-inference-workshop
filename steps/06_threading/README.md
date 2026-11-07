# Step 6: Inference on a worker thread

**Goal:** get the engine off the audio thread and keep the latency fixed.

**What it answers:** step 5 showed that the engine may not run in the callback. This is where the inference goes instead.

## The shape

```
audio thread                     worker thread
------------                     -------------
process_block()                  process_model(), over and over
  push ──→ m_input  ───────────→   pop 2048 samples
                                   engine.process()
  pop  ←── m_output ←───────────   push 2048 samples
           (starts with the prefill)
```

Step 3's `ProcessorExample`, split over two threads: `process_block` keeps the ring buffers, `process_model` gets the model. [`ThreadSafeRingBuffer`](../common/helpers/threadsafe_ring_buffer.h) is safe for one writer and one reader on different threads. It has `try_push` and `try_pop`, which move one sample and say whether it worked. Neither waits, throws or allocates: the audio thread gives up when one fails, the worker tries again right away. Underneath is [moodycamel's](https://github.com/cameron314/readerwriterqueue) single-producer single-consumer circular buffer.

The thread itself is given: [`WorkerThread`](../common/helpers/worker_thread.h) calls `process_model()` over and over from `start()` until `stop()`.

## The prefill

The worker needs time, and the host must always find something to take. So `m_output` starts with silence, the prefill: step 3's prefill, 2048 − 512, plus the worker's time. Working it out is TODO 1, and step 7 takes it apart.

The worker's time is `worker_time`, given in `prepare()`: your worst case from step 4, `k_max_inference_ms` in [`common/inference_time.h`](../common/inference_time.h), which `main` passes into `prepare()` as `max_inference_ms`, in samples and rounded up to whole callbacks, because the audio thread only picks results up once per callback. Two lines at the top of `prepare()` do it: 18 ms are 794 samples at 44.1 kHz, two 512-sample callbacks, 1024. Until you do, it is the largest worst case that can work at all, one model block of time, 46.4 ms: everything runs, with more latency.

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp) and fill in the three TODO banners.

1. **In `prepare()`**: the prefill, step 3's plus the worker's time. The ring buffer sizes are given: `m_input` one model block, `m_output` the worker's time plus one model block.
2. **In `process_block()`**, the audio thread: step 3's `process_block`, without the engine, one sample at a time:
   - push into `m_input` with `try_push()`;
   - pop from `m_output` with `try_pop()`.
3. **In `process_model()`**, the worker thread: the inference is given. Around it, pop a whole model block from `m_input` and push the result into `m_output`, sample by sample. When `try_pop()` or `try_push()` fails, try again right away, a hot spin, until it worked.

Given, around your lines: the worker's time, the ring buffer sizes, starting and stopping the worker, and what the audio thread does when something doesn't work:

- `m_input` is full: the worker is a whole block behind. The rest is counted in `m_unpushed`, the host gets that many zeros instead, and nothing is taken out of `m_output` for them.
- A sample is not in `m_output` yet, or something is still owed: the worker is late. The rest of the callback goes out as zeros, all of it owed, `m_debt`. Not just that one sample: the worker may be pushing right now, and the next one would land one place too early.
- The debt is paid first: an owed sample came too late, its place already went out as a zero, so it is taken out and thrown away.

The exercise builds once every `?` is filled in. Then each TODO says so while it is off:

```
  FAIL  TODO 1: the prefill is too small, the worker's first block would come too late
  FAIL  TODO 1: the prefill is too large, the host would hear the model later than needed
  FAIL  TODO 2 or 3: the host only got silence. ...
```

Each is followed by `STEP 6 NOT SOLVED`.

The checks behind these messages live in [`common/checks/step06.h`](../common/checks/step06.h), so `main.cpp` shows only the exercise.

## What you get

```
  prefill                      3584 samples (81.3 ms)
  PASS  the output matches the reference: max abs diff 2.91e-07, tolerance 0.0001
  PASS  inference ran on the worker thread, the host got the model's stream back
        3584 samples late

==============================================================
  STEP 6 SOLVED
==============================================================
```

That is with `k_max_inference_ms` still at 46.4 ms; with your number it will be smaller. The check listens like the host: first the prefill in silence, then the model's stream, to about 3e-07 of the reference. A callback with zeros where the reference is not silent is a dropout: the worker was late. More than a quarter of them fail the run, and so does any sample that is neither the reference nor a zero.

## The host runs in real time now

`main` is step 3's host loop with one line more: a sleep after every callback, for as long as one block of audio lasts, 11.6 ms for 512 samples at 44.1 kHz ([`block_duration`](../common/helpers/host.h)). Two threads only work if the audio thread is called at the audio rate, the way a device calls it. Up to now the loop ran as fast as it could, which no worker thread could ever keep up with.

## What the audio thread still does

Look at what is left in `process_block`: pushing and popping samples. No engine, no `new`, no mutex, no waiting.

And what it does when the worker is late: it never waits. The host gets zeros, a dropout, and the samples that came too late are thrown away when they arrive, so the stream stays exactly the prefill late. Without that, every late block would make the output a little later for good.

## Slides

[`slides/06-threading.md`](../../slides/06-threading.md).

## What's next

The audio thread is safe and the host hears the model's stream, the prefill late. But the model itself answers late too, and the host has to be told one number for all of it, with a dry signal that lines up: that is the next step.
