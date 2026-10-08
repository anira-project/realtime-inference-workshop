<h1>Step 3<br>Benchmarking and<br>the real-time budget</h1>

How long does it take — and how long in the worst case?

<!-- .slide: data-state="no-header" -->

Note:
    - Ask the room: who measures worst case rather than average? What do you measure with?

---

## The deadline

A callback has to be done before the host wants the next block:
`budget = host block size / sample rate`

| host block | 44.1 kHz | 48 kHz | 96 kHz |
|---|---|---|---|
| 64 | 1.45 ms | **1.33 ms** | 0.67 ms |
| 128 | 2.90 ms | 2.67 ms | 1.33 ms |
| 512 | 11.6 ms | 10.7 ms | 5.33 ms |
| 2048 | 46.4 ms | 42.7 ms | 21.3 ms |

Miss it once and you hear it.

---

## Two block sizes, one deadline

Our model takes **2048 samples**: 42.7 ms of *audio*.

The host hands us **64 samples**: 1.33 ms of *time*.

```
callback:  |64|64|64| ... |64|   ← 1.33 ms each
model:     .  .  .       [2048]  ← runs in one of them
```

31 callbacks do nothing. The 32nd has to run the whole forward pass — and still
has 1.33 ms.

Note:
    - This is the slide to linger on. The audio duration of the model block and the deadline of a callback are unrelated numbers.

---

## What that costs

Forward pass on this laptop: **≈ 5 ms**.

- Average load: 5 ms of work per 42.7 ms of audio → **12%**
- The callback that runs it: 5 ms of work in a 1.33 ms budget → **375%**

A CPU meter showing 12% and an audio stream that clicks every 43 ms.

---

## Mean is not the number

```
forward_pass_mean       5.13 ms
forward_pass_median     4.57 ms
forward_pass_p95        5.21 ms
forward_pass_p99        5.36 ms
forward_pass_max        84.5 ms      ← one call in 200
```

Page faults, the allocator, a thermal step, the scheduler. The mean is stable
across runs; **the max is not, and it is the one that decides**.

Note:
    - Ask: what is your worst-case budget in practice? Collect numbers — most people only know the average.

---

## Your job

1. **Run the model in the timed loop**
2. **Implement `percentile()`** — p95, p99
3. **One call per repetition** — not an average of many

```bash
./build/bin/step03_exercise
```

Run it three times and watch which numbers move.

---

## Reading the result

- **mean / median** — what a CPU meter shows
- **p95 / p99** — what most users hit sometimes
- **max** — what decides whether you shipped a broken plugin

Rule of thumb: the worst case has to fit the budget, with room to spare,
because everything else in the host is also fighting for that deadline.

---

## What broke?

Nothing yet — we measured on an ordinary thread, offline.

Next: put that call in the audio callback, where allocations, locks and page
faults are not slow but **fatal**.

<!-- .slide: data-state="no-footer" -->

Note:
    - Hand over to real-time safety: it is not only about being fast enough.
