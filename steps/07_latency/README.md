# Step 7: Latency and the dry/wet mix

**No exercise here** — this one is a demonstration: two lines in `prepare()`, walked through on the slides.

**Goal:** work out how late the processed signal is, report that number, and delay the dry signal by the same amount so the two can be mixed.

**Problem it solves:** step 6 left the audio thread safe and the stream exactly the prefill late, but the model itself answers late too. A plugin that cannot say how late it is cannot be mixed, and cannot be lined up by the host.

## Where the delay comes from

Nothing here is the engine being slow. It is the design, and the model:

| | samples | why |
|---|---|---|
| buffer adaptation | 2048 − 512 = 1536 | the model cannot run before a whole block has arrived, and the host brings 512 at a time (step 3) |
| the worker's time | `rounded_max_inference_samples`, your step 4 number in whole 512-sample callbacks: 1024 for 11.6 to 23.2 ms | the time the worker is given (step 6) |
| the model itself | 2224 | its output answers its input this much later: 1696 from the decoder's transposed convolutions, 528 from the PQMF filter bank. `k_model.m_latency`, computed by `scripts/export_model.py` |
| **total** | **4784** with 11.6 to 23.2 ms | 108.5 ms at 44.1 kHz |

The first two are step 6's prefill. The second is a choice: it buys the worker the time of your worst case, and in exchange the latency is a **fixed number** instead of whatever the worker managed today. A number that changes with the weather is useless to the host. And it only holds for a host block of 512: other block sizes give other numbers, which is part 2.

## What it does

[`solution/main.cpp`](solution/main.cpp) is step 6's `ProcessorExample`, filled in, with a dry path next to it: `m_dry`, step 3's ring buffer, which only the audio thread uses. Your number from step 4 comes from [`common/inference_time.h`](../common/inference_time.h), as in step 6. Two lines in `prepare()` are what this step is about:

1. **`m_latency = m_prefill + m_model.m_latency`**: the number above, what the host is told. Step 6's prefill holds the first two parts; the model's own delay comes on top. The wet path only gets the prefill: the model's own delay is already inside what it returns.
2. **The dry delay**: `m_dry` starts with the full latency in silence.

The mix in `process_block()` is a plain crossfade between the delayed dry signal and the wet one: both paths are equally late. The check behind the output lives in [`common/checks/step07.h`](../common/checks/step07.h).

## What it prints

```
  reported latency             5808 samples (131.7 ms at 44100 Hz)
  PASS  dry and wet are both late by exactly the reported latency, so they line up:
        max abs diff 1.49e-07 at mix 0.5

==============================================================
  STEP 7 SOLVED
==============================================================
```

That is with `k_max_inference_ms` still at 46.4 ms; with your number it will be smaller: 4784 samples from 11.6 to 23.2 ms. Every callback more or less of worst case changes it by 512. The host runs as in step 6, at a mix of 0.5. The check builds what the host should hear: half the input, delayed by the reported latency, plus half the model's stream, delayed by the rest of it (the reference already carries the model's own delay). If either path is off by a single sample, the two do not line up and the run fails. A callback where the worker was late has only the dry half: that is a dropout, and more than a quarter of them fail the run.

So a wrong latency number cannot pass.

## Why the dry path needs a delay line at all

Mixing an undelayed dry signal with a wet one that is 108 ms late does not sound like a blend; it sounds like a slap-back echo, and at low mix values like comb filtering. The dry path gets its own ring buffer, primed with the full latency in silence, so both arrive together.

In a plugin you report the same number with `setLatencySamples()`, and the host lines the track up with everything else.

## Slides

[`slides/07-latency.md`](../../slides/07-latency.md).

## What's next

Everything is in place: correct, safe, late by a known amount. Step 8 puts it into a JUCE plugin, where the host asks for exactly these numbers.
