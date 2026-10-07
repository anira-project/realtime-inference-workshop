<h1>Step 7<br>Latency and the<br>dry/wet mix</h1>

The output is late. By how much, exactly?

<!-- .slide: data-state="no-header" -->

Note:
    - Ask first: who reports latency in their plugins today, and where did the number come from?

---

## Where the delay comes from

Not the engine being slow — the design:

| | samples |
|---|---|
| collecting a block | 2048 |
| handing it over, one block of slack | 2048 |
| **total** | **4096** = 85.3 ms at 48 kHz |

The second block is a **choice**: it buys the worker a full block of time, and
in exchange the latency is a fixed number.

---

## Fixed beats small

A latency that changes with CPU load is not a latency, it is a guess.

- The host gets one number, once, and lines the track up with it
- Everything else in the session is aligned to it
- If it changes mid-stream, you get a click at best

So: pick the number the design can **always** hold, and pad to it.

---

## Dry and wet have to meet

```
input ──┬── [ ring buffer, 4096 ] ─────────────── dry ──┐
        │                                               ├──► mix
        └── [ collect ] → [ worker: model ] → wet ──────┘
```

Mix an undelayed dry with an 85 ms wet and you do not get a blend — you get a
slap-back, and comb filtering on the way there.

---

## Your job

1. **`latency_samples()`** — one number, three jobs: what the host is told, how
   much silence both paths start with, how far dry is delayed
2. **The mix** — both sides are equally late now, so it is a crossfade

```bash
./build/bin/step07_exercise
```

---

## How we know the number is right

```
  dry, delayed by the latency  max abs diff 0          OK
  wet, delayed by the latency  max abs diff 3.51e-06   OK
```

- **100% dry has to come out at exactly 0.0** — it is the input, only later.
  One sample off and this jumps
- **100% wet matches the reference** to 1e-6

A wrong latency cannot pass this. Same trick in a plugin: bypass the model, and
the signal must come out unchanged apart from the delay.

Note:
    - This is the slide to insist on: latency is testable, not a thing you estimate once and hope for.

---

## In a plugin

```cpp
void prepareToPlay(double sample_rate, int max_block) override {
    processor.prepare(max_block);
    setLatencySamples(processor.latency_samples());
}
```

The host does the rest: delay compensation for every other track.

---

## What broke?

Nothing. Correct, real-time safe, late by a known amount.

And look at what it took: ring buffers, a worker thread, two queues, a prefill
rule, a latency calculation, and a dry path to match.

Next block: what anira does with all of this.

<!-- .slide: data-state="no-footer" -->

Note:
    - Hand over to the break and the anira architecture talk.
