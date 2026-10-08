<h1>Step 7<br>Latency and the<br>dry/wet mix</h1>

The output is late. By how much, exactly?

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
    - Ask first: who reports latency in their plugins today, and where did the number come from?

---

## Why: fixed beats small

A latency that changes with CPU load is not a latency, it is a guess.

- The host gets one number, once, and lines the track up with it
- Everything else in the session is aligned to it
- If it changes mid-stream, you get a click at best

So: pick the number the design can **always** hold, and pad to it.

---

## Why: dry and wet have to meet

```plaintext
input ──┬── [ ring buffer, delayed ] ──────────── dry ──┐
        │                                               ├──► mix
        └── [ collect ] → [ worker: model ] → wet ──────┘
```

Mix an undelayed dry with a late wet and you do not get a blend — you get a
slap-back, and comb filtering on the way there.

---

## Goal

The processor reports **exactly** how late it is, and dry and wet line up in the mix.

1. **`latency_samples()`** — one number, three jobs: what the host is told, how much
   silence both paths start with, how far dry is delayed
2. **The mix** — both sides are equally late now, so it is a crossfade

---

## What's given

```plaintext
models/forward_stateful.pt    the model, as in step 1
WORKSHOP_MODEL_PATH           its path, set by CMake → k_model.m_path
common/libtorch_engine.h      the engine from step 1
common/ring_buffer.h          from step 2
common/host.h                 from step 2, in real time, 512 samples per call
readerwriterqueue.h           the lock-free queue from step 6
exercise/main.cpp             LatencyProcessor, two TODOs
```

`LatencyProcessor` is step 6's processor plus a **dry path**: a third ring buffer,
primed with `latency_samples()` of silence in `prepare()`, and `set_mix()`.

---

## Task

<div class="task-timer" data-minutes="5"></div>

Fill in the TODOs in `steps/07_latency/exercise/main.cpp`

```bash
cmake --build --preset release --target step07_exercise
./build/bin/step07_exercise
```

<div class="nn-flow task-flow">
  <div class="nn-node">fake host<small>512 samples, in real time</small></div>
  <div class="nn-arrow">→</div>
  <div class="nn-stack">
    <div class="nn-node exercise">dry: delayed<small><code>latency_samples()</code></small></div>
    <div class="nn-node exercise">wet: worker + model<small>late by the design</small></div>
  </div>
  <div class="nn-arrow">→</div>
  <div class="nn-node exercise">mix</div>
  <div class="nn-arrow">→</div>
  <div class="nn-stack">
    <div class="nn-node check">100% dry<small>= input, delayed exactly</small></div>
    <div class="nn-node check">100% wet<small>= target, delayed exactly</small></div>
  </div>
</div>

---

## If you see this: perfect

```plaintext
Reported latency: 4096 samples (85.3 ms at 48000 Hz)

  dry, delayed by the latency max abs diff 0          OK
  wet, delayed by the latency max abs diff 3.51e-06   OK
```

- **100% dry has to come out at exactly 0.0** — it is the input, only later. One
  sample off and this jumps
- **100% wet matches the reference**

A wrong latency cannot pass this.

Note:
    - This is the slide to insist on: latency is testable, not a thing you estimate once and hope for.

---

## TODO 1 — the latency

```cpp
size_t latency_samples() const { return 2 * k_model.m_input_size; }   // 4096
```

| | samples |
|---|---|
| collecting a block | 2048 |
| handing it over, one block of slack | 2048 |
| **total** | **4096** = 85.3 ms at 48 kHz |

The second block is a **choice**: it buys the worker a full block of time, and in
exchange the latency is a fixed number.

---

## TODO 2 — the mix

```cpp
std::fill_n(samples, num_samples, 0.0f);

if (m_dry.available() >= num_samples) {
    m_dry.pop(m_dry_block.data(), num_samples);
    for (size_t i = 0; i < num_samples; ++i) { samples[i] += (1.0f - m_mix) * m_dry_block[i]; }
}

if (m_output.available() >= num_samples) {
    m_output.pop(m_wet_block.data(), num_samples);
    for (size_t i = 0; i < num_samples; ++i) { samples[i] += m_mix * m_wet_block[i]; }
}
```

If the wet side has nothing ready, the dry side still has to come through.

---

## In a plugin

```cpp
void prepareToPlay(double sample_rate, int max_block) override {
    processor.prepare(max_block);
    setLatencySamples(processor.latency_samples());
}
```

The host does the rest: delay compensation for every other track. Same test as
here: bypass the model, and the signal must come out unchanged apart from the delay.

---

## What broke?

Nothing. Correct, real-time safe, late by a known amount.

And look at what it took: ring buffers, a worker thread, two queues, a prefill
rule, a latency calculation, and a dry path to match.

Next: all of it in a plugin.

<!-- .slide: data-state="no-footer" -->

Note:
    - Hand over to step 8: the same processor inside JUCE.
