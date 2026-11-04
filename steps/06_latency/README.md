# Step 6: Latency and the dry/wet mix

**Goal:** work out how late the processed signal is, report that number, and delay the dry signal by the same amount so the two can be mixed.

**Problem it solves:** step 5 left the audio thread safe and the stream exactly the prefill late, but the model itself answers late too. A plugin that cannot say how late it is cannot be mixed, and cannot be lined up by the host.

## Where the delay comes from

Nothing here is the engine being slow. It is the design, and the model:

| | samples | why |
|---|---|---|
| buffer adaptation | 2048 − 512 = 1536 | the model cannot run before a whole block has arrived, and the host brings 512 at a time (step 2) |
| rounded max inference time | your step 3 number, rounded up to whole 512-sample callbacks: 512 for up to 11.6 ms | the time the worker is given (step 5) |
| the model itself | 2224 | its output answers its input this much later: `k_model.m_latency`, computed by `scripts/export_model.py` |
| **total** | **4272** with up to 11.6 ms | 96.9 ms at 44.1 kHz |

The first two are step 5's prefill. The second is a choice: it buys the worker the time of your worst case, and in exchange the latency is a **fixed number** instead of whatever the worker managed today. A number that changes with the weather is useless to the host. And it only holds for a host block of 512: other block sizes give other numbers, which is part 2.

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp). First put your number from step 3 into `k_max_inference_ms`, as in step 5. Then fill in the two TODO banners.

1. **`latency_samples()`** — the number above. It decides what the host is told and how far the dry signal is delayed. The wet path only gets step 5's prefill: the model's own delay is already inside what it returns.
2. **The dry delay** in `prepare()` — prefill the dry ring buffer with the full latency in silence.

The mix in `process_block()` is given: both paths are equally late, so it is a plain crossfade between the delayed dry signal and the wet one.

Each TODO says so while it is still open:

```
TODO 1: put your worst case from step 3 into k_max_inference_ms.
TODO 1: work out the latency the design introduces.
TODO 2: the dry signal comes out undelayed, so it cannot line up with the wet one.
```

## Expected output

```
Reported latency: 4272 samples (96.9 ms at 44100 Hz)

  dry, delayed by the latency max abs diff 0          OK
  wet, delayed by the latency:
OK: max abs diff 3.28e-07, within 0.0001 of the reference.

OK: both paths are late by exactly the reported latency, so they mix.
```

With a worst case above 11.6 ms your latency is larger, by 512 for every further callback. The check is the point: the program runs the signal twice, once at 100% dry and once at 100% wet, and compares each against its reference *shifted by the reported latency*.

- **Dry has to match exactly**, 0.0 difference. It is the input, only later. If the latency is off by a single sample, this number jumps.
- **Wet matches to 1e-6**, the usual difference between LibTorch here and ONNX Runtime in Python. The reference already carries the model's own delay, so it is shifted by the rest: the prefill. Leave the model's part out of the latency and this check fails. A callback where the worker was late goes out with zeros and is counted as a dropout; more than a quarter of them fail the run.

Which means: a wrong latency number cannot pass. That is also how you would catch it in a plugin — bypass the model, and the signal has to come out unchanged apart from the delay.

## Why the dry path needs a delay line at all

Mixing an undelayed dry signal with a wet one that is 97 ms late does not sound like a blend; it sounds like a slap-back echo, and at low mix values like comb filtering. The dry path gets its own ring buffer, primed with the full latency in silence, so both arrive together.

In a plugin you report the same number with `setLatencySamples()`, and the host lines the track up with everything else.

## Slides

[`slides/06-latency.md`](../../slides/06-latency.md).

## What's next

Everything is in place: correct, safe, late by a known amount. Step 7 decides which engine to ship, and step 8 puts it into a JUCE plugin, where the host asks for exactly these numbers.
