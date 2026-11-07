# Step 3: Model input size vs. host buffer size

**Goal:** feed a model that only accepts 2048 samples from a host that hands you 512 at a time.

**Problem it reveals:** the two sizes have nothing to do with each other. Something has to sit in between.

## Why this is not a corner case

The model's block size comes from the export. The host's comes from the audio device, the user's settings, and the sample rate. They match by accident, once. In this part of the workshop the host always hands over 512 samples: not the model's 2048, but a divisor of it, so four callbacks make one model block. Arbitrary host block sizes need more than this, and part 2 shows how.

## What you have

- [`common/helpers/ring_buffer.h`](../common/helpers/ring_buffer.h) — a ring buffer, written for you: `try_push` and `try_pop` move one sample and say whether it worked, `available` and `space` say how full it is.
- `common/helpers/onnx_engine.h` — the engine from step 1, unchanged, with the state from step 2.
- `common/helpers/support.h`, `common/assets/test_signal.h`, `common/assets/target_signal.h` — as before.
- [`common/checks/step03.h`](../common/checks/step03.h) — `check_prefill`, called in `prepare`, and `check_todos`, called at the end of `process_block`: say which TODO is off. Not part of the exercise.

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp) and fill in `ProcessorExample`, at the three TODO banners. It has the shape a plugin has:

```cpp
ProcessorExample processor(k_model);              // Keeps the model, makes its engine, loads nothing
processor.prepare(k_host_block_size);             // Before the audio starts: load, allocate
processor.process_block(samples, num_samples);    // Per callback, in place
```

The processor is made with the model and owns the engine: it keeps `k_model` as `m_model`, and inside the class the sizes come from `m_model`. `prepare` loads the model, once. The host's block size is fixed, 512 samples, for the whole run.

1. **The prefill**, in `prepare()`: how many zeros `m_output` starts with, so the host finds samples in every callback, also before the model's first block. The host takes 512 per callback, the model gives back 2048 once a whole block is in. The ring buffers' sizes follow from it and are given; everything that allocates belongs here, not in the callback.
2. **The pump**, in `process_block()`, one sample at a time. Push the host's samples into the input ring buffer, check whether a whole model block is waiting there, and if so pop it into `m_model_block`, run the model (given) and push the result into the output ring buffer. What is left over waits for the next call.
3. **Give the host its samples back**, in place: pop them from the output ring buffer. At the start they are the prefill's zeros.

Build and run from the repository root:

```bash
cmake --preset release
cmake --build --preset release --target step03_exercise
./build/release/bin/step03_exercise
```

## Expected output

```
OK: max abs diff 3.87e-07, within 0.0001 of the reference.
```

`main` plays the host: a loop that hands `process_block` 512 samples at a time, in place, like the loop in step 2. What came back has to be the model's stream from step 1, sample for sample, after a stretch of silence.

## If you're stuck

The exercise builds once every `?` is filled in. Then each TODO says so when it is off:

- **`TODO 1: the prefill is too small, …`** — the host would find `m_output` empty before the model's first block is in. Count the samples the host takes until then.
- **`TODO 1: the prefill is too large, …`** — it works, but the host hears the model later than it has to.
- **`TODO 2: run the model on the samples that arrived`** — the input is piling up: a whole model block is waiting, but the check for it is not filled in yet, or the block is not popped from `m_input`.
- **`TODO 3: give the host its samples back`** — the output is piling up instead of being popped into the host's buffer.

- **`FAILED: max abs diff …`** — every TODO is filled in, but the host did not get the model's stream back: the samples are not the right ones, or not in the right place.

Compare with [`solution/main.cpp`](solution/main.cpp) when you want to.

## Slides

[`slides/03-host-buffer-size.md`](../../slides/03-host-buffer-size.md).

## What broke?

Nothing, but two things are now true that were not before. The host hears the prefill first, 1536 samples of silence: that is part of the latency, the question after the solution. And every fourth callback now runs a whole forward pass. How long does that take, and what is the budget? That is step 4.
