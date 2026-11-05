# Step 6: Inference on a worker thread

**Goal:** get the engine off the audio thread and keep the latency fixed.

**What it answers:** step 5 showed that neither engine may run in the callback. This is where the inference goes instead.

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

Step 3's `ProcessorExample`, split over two threads: `process_block` keeps the ring buffers, `process_model` gets the model. [`ThreadSafeRingBuffer`](../common/helpers/threadsafe_ring_buffer.h) is safe for one writer and one reader on different threads: step 3's `push`, `pop` and `available` for the worker's blocks, and `try_push` and `try_pop` for the audio thread, which move one sample and say whether it worked. None of them waits, throws or allocates. Underneath is [moodycamel's](https://github.com/cameron314/readerwriterqueue) single-producer single-consumer circular buffer.

The thread itself is given: [`WorkerThread`](../common/helpers/worker_thread.h) calls `process_model()` over and over from `start()` until `stop()`.

## The prefill

The worker needs time, and the host must always find something to take. So `m_output` starts with silence, as many samples as the host is late:

```
prefill = (2048 − 512)                  buffer adaptation, as in step 3
        + rounded_max_inference_time    the worker's time
```

`rounded_max_inference_time` is your worst case from step 4 in samples, rounded up to whole 512-sample callbacks: the audio thread only picks results up once per callback. It comes from `k_max_inference_ms` in [`common/inference_time.h`](../common/inference_time.h), where you put your number from step 4. Until you do, it is the largest worst case that can work at all, one model block of time, 46.4 ms: everything runs, with more latency.

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp) and fill in the three TODO banners.

1. **In `prepare()`**: work out `rounded_max_inference_time`, the prefill and the size of `m_output` (the worker's time plus one model block).
2. **In `process_block()`**, the audio thread: step 3's `process_block`, without the engine, one sample at a time:
   - push into `m_input`. If it is full, the worker is a whole block behind: count the rest in `m_unpushed`, the host gets that many zeros instead (given), and nothing is taken out of `m_output` for them;
   - pop from `m_output`. If a sample is not there yet, or something is still owed, the worker is late: the rest of the callback goes out as zeros, all of it owed, `m_debt`. Not just that one sample: the worker may be pushing right now, and the next one would land one place too early;
   - pay the debt first: an owed sample came too late, its place already went out as a zero, so it is taken out and thrown away.
3. **In `process_model()`**, the worker thread: if `m_input` holds a whole model block, pop it, run the model (given) and push the result into `m_output`.

Given, around your lines: the warm-up, and starting and stopping the worker.

Each TODO says so while it is still open:

```
FAILED: TODO 1: round k_max_inference_ms up to whole callbacks, in samples
FAILED: TODO 1: work out the prefill
FAILED: TODO 1: give m_output room for the prefill and a callback more
FAILED: TODO 2 or 3: the host only got silence. ...
```

The checks behind these messages live in [`common/checks/step06.h`](../common/checks/step06.h), so `main.cpp` shows only the exercise.

## What you get

```
  prefill                      2560 samples (58.0 ms)
OK: max abs diff 3.87e-07, within 0.0001 of the reference.
OK: inference ran on the worker thread, the host got the model's stream back
2560 samples late.
```

The prefill is 2560 for a worst case of 11.6 to 23.2 ms; yours may differ. The check listens like the host: first the prefill in silence, then the model's stream, to 3.87e-07 of the reference. A callback with zeros where the reference is not silent is a dropout: the worker was late. More than a quarter of them fail the run, and so does any sample that is neither the reference nor a zero.

## The host runs in real time now

`main` is step 3's host loop with one line more: a sleep after every callback, for as long as one block of audio lasts, 11.6 ms for 512 samples at 44.1 kHz ([`block_duration`](../common/helpers/host.h)). Two threads only work if the audio thread is called at the audio rate, the way a device calls it. Up to now the loop ran as fast as it could, which no worker thread could ever keep up with.

## What the audio thread still does

Look at what is left in `process_block`: pushing and popping samples. No engine, no `new`, no mutex, no waiting.

And what it does when the worker is late: it never waits. The host gets zeros, a dropout, and the samples that came too late are thrown away when they arrive, so the stream stays exactly the prefill late. Without that, every late block would make the output a little later for good.

## Slides

[`slides/06-threading.md`](../../slides/06-threading.md).

## What's next

The audio thread is safe and the host hears the model's stream, the prefill late. But the model itself answers late too, and the host has to be told one number for all of it, with a dry signal that lines up: that is the next step.
