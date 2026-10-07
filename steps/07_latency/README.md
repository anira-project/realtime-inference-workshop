# Step 7: Latency and the dry/wet mix

**Goal:** work out how late the processed signal is, report that number, and delay the dry signal by the same amount so the two can be mixed.

**Problem it solves:** step 6 left the audio thread safe but the output late. A plugin that cannot say how late it is cannot be mixed, and cannot be lined up by the host.

## Where the delay comes from

Nothing here is the engine being slow. It is the design:

| | samples | why |
|---|---|---|
| collecting a block | 2048 | the model cannot run before a whole block has arrived |
| handing it over | 2048 | the result is picked up in a later callback, so the output side starts primed with silence |
| **total** | **4096** | 85.3 ms at 48 kHz |

The second block is a choice: it buys the worker a full block of time to answer, and in exchange the latency is a **fixed number** instead of whatever the worker managed today. A number that changes with the weather is useless to the host.

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp) and fill in the two TODO banners.

1. **`latency_samples()`** — the number above. It decides three things at once: what the host is told, how much silence both paths start with, and how far the dry signal is delayed.
2. **The mix** in `process_block()` — both paths are equally late now, so it is a plain crossfade between the delayed dry signal and the wet one.

Each TODO says so while it is still open:

```
TODO 1: work out the latency the design introduces.
TODO 2: mix dry and wet, and hand the result to the host.
```

## Expected output

```
Reported latency: 4096 samples (85.3 ms at 48000 Hz)

  dry, delayed by the latency  max abs diff 0          OK
  wet, delayed by the latency  max abs diff 3.51e-06   OK

OK: both paths are late by exactly the reported latency, so they mix.
```

The check is the point: the program runs the signal twice, once at 100% dry and once at 100% wet, and compares each against its reference *shifted by the reported latency*.

- **Dry has to match exactly**, 0.0 difference. It is the input, only later. If the latency is off by a single sample, this number jumps.
- **Wet matches to 1e-6**, the usual difference between LibTorch here and ONNX Runtime in Python.

Which means: a wrong latency number cannot pass. That is also how you would catch it in a plugin — bypass the model, and the signal has to come out unchanged apart from the delay.

## Why the dry path needs a delay line at all

Mixing an undelayed dry signal with a wet one that is 85 ms late does not sound like a blend; it sounds like a slap-back echo, and at low mix values like comb filtering. The dry path gets its own ring buffer, primed with the same silence, so both arrive together.

In a plugin you report the same number with `setLatencySamples()`, and the host lines the track up with everything else.

## Slides

[`slides/07-latency.md`](../../slides/07-latency.md).

## What's next

Everything is in place: correct, safe, late by a known amount. Step 8 puts it into a JUCE plugin, where the host asks for exactly these numbers.
