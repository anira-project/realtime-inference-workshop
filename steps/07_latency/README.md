# Step 7: Latency and the dry/wet mix

**Goal:** work out how late the processed signal is, report that number, and delay the dry signal by the same amount so the two can be mixed.

**Problem it solves:** step 6 left the audio thread safe and the stream exactly the prefill late, but the model itself answers late too. A plugin that cannot say how late it is cannot be mixed, and cannot be lined up by the host.

## Where the delay comes from

Nothing here is the engine being slow. It is the design, and the model:

| | samples | why |
|---|---|---|
| buffer adaptation | 2048 − 512 = 1536 | the model cannot run before a whole block has arrived, and the host brings 512 at a time (step 3) |
| rounded max inference time | your step 4 number, rounded up to whole 512-sample callbacks: 1024 for 11.6 to 23.2 ms | the time the worker is given (step 6) |
| the model itself | 2224 | its output answers its input this much later: `k_model.m_latency`, computed by `scripts/export_model.py` |
| **total** | **4784** with 11.6 to 23.2 ms | 108.5 ms at 44.1 kHz |

The first two are step 6's prefill. The second is a choice: it buys the worker the time of your worst case, and in exchange the latency is a **fixed number** instead of whatever the worker managed today. A number that changes with the weather is useless to the host. And it only holds for a host block of 512: other block sizes give other numbers, which is part 2.

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp). It is step 6's `ProcessorExample`, filled in, with a dry path next to it: `m_dry`, step 3's ring buffer, which only the audio thread uses. First put your number from step 4 into `k_max_inference_ms`, as in step 6. Then fill in the two TODO banners, both in `prepare()`:

1. **`m_latency`**: the number above, what the host is told. Step 6's prefill already holds the first two parts. The wet path only gets that prefill: the model's own delay is already inside what it returns.
2. **The dry delay**: prefill `m_dry` with the full latency in silence.

The mix in `process_block()` is given: both paths are equally late, so it is a plain crossfade between the delayed dry signal and the wet one.

Each TODO says so while it is still open:

```
FAILED: put your worst case from step 4 into k_max_inference_ms
FAILED: TODO 1: work out the latency
FAILED: TODO 2: delay the dry signal: m_dry has to start with the latency in silence
```

The checks behind these messages live in [`common/checks/step07.h`](../common/checks/step07.h).

## Expected output

```
Reported latency: 4784 samples (108.5 ms at 44100 Hz)
OK: max abs diff 1.94e-07 at mix 0.5: dry and wet are both late by exactly the
reported latency, so they line up.
```

With a different worst case your latency differs, by 512 for every callback. The host runs as in step 6, at a mix of 0.5. The check builds what the host should hear: half the input, delayed by the reported latency, plus half the model's stream, delayed by the rest of it (the reference already carries the model's own delay). If either path is off by a single sample, the two do not line up and the run fails. A callback where the worker was late has only the dry half: that is a dropout, and more than a quarter of them fail the run.

So a wrong latency number cannot pass.

## Why the dry path needs a delay line at all

Mixing an undelayed dry signal with a wet one that is 108 ms late does not sound like a blend; it sounds like a slap-back echo, and at low mix values like comb filtering. The dry path gets its own ring buffer, primed with the full latency in silence, so both arrive together.

In a plugin you report the same number with `setLatencySamples()`, and the host lines the track up with everything else.

## Slides

[`slides/07-latency.md`](../../slides/07-latency.md).

## What's next

Everything is in place: correct, safe, late by a known amount. Step 8 puts it into a JUCE plugin, where the host asks for exactly these numbers.
