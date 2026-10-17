<h1>Threads and latency</h1>

One pool, no waiting, one number

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - Three concepts, each in a pair of slides: the step in part 1 that built it by hand, then what anira does instead.

---

<div class="tag">Part 1 · Step 6</div>

## A worker per channel,<br><span class="then">per instance.</span>

<div class="lanes">
  <div class="lane-label">audio thread</div>
  <div class="lane audio"></div>
  <div class="lane-label"></div>
  <div class="lane-gap">
    <div class="lane-queue to"><span>to_worker</span><i></i></div>
    <div class="lane-queue from"><span>from_worker</span><i></i></div>
  </div>
  <div class="lane-label">worker thread</div>
  <div class="lane worker"><div class="lane-run">model</div></div>
</div>

Note:
    - Step 6's picture. One worker, two queues, one engine. Correct, and it does not scale: the worker belongs to the instance, and the host decides how many instances there are.

---

<div class="tag">anira · Threads</div>

## One pool per process.<br><span class="then">Every instance queues into it.</span>

<div class="pool">
  <div class="pool-instances">
    <div class="pool-instance" style="--i:0">instance 1</div>
    <div class="pool-instance" style="--i:1">instance 2</div>
    <div class="pool-instance" style="--i:2">instance 3</div>
    <div class="pool-instance" style="--i:3">…</div>
    <div class="pool-instance" style="--i:4">instance 10</div>
  </div>
  <div class="pool-queue"><span>one queue</span><div class="q small"><div class="q-slot"><b></b></div><div class="q-slot"><b></b></div><div class="q-slot"><b></b></div><div class="q-slot"></div></div></div>
  <div class="pool-threads">
    <div class="pool-thread" style="--i:0">thread 1</div>
    <div class="pool-thread" style="--i:1">thread 2</div>
    <div class="pool-thread" style="--i:2">thread 3</div>
    <div class="pool-thread" style="--i:3">thread 4</div>
  </div>
</div>

```cpp
context.threads(4, ANIRA_WAIT_SPIN_BACKOFF);   // once per process; the default: cores / 2
```

Note:
    - The Context is process-wide: the first handler creates the pool, the last one releases it. Ten plugins, one pool, four real-time threads.
    - One lock-free queue (moodycamel) for every handler. The audio thread enqueues a job and leaves; a free thread takes the oldest one.
    - A stateful model never has two jobs in flight: its next job is enqueued by the thread that finished the last one, so the order holds without a lock.
    - Thread count: hardware concurrency / 2 by default. Zero means the host brings its own threads.

---

<div class="tag">Part 1 · Step 6</div>

## The worker polls,<br><span class="then">the audio thread never waits.</span>

```cpp
// the worker
if (!m_to_worker.try_dequeue(block)) { sleep_for(100us); continue; }

// the audio thread
if (m_output.available() >= num_samples) { m_output.pop(samples, num_samples); }
else                                     { std::fill_n(samples, num_samples, 0.0f); }
```

Note:
    - Two decisions we made without naming them: how the worker waits for work, and that the audio thread never waits for the worker.
    - 100 µs sleeps are a compromise: responsive enough, and it burns a little CPU while idle.

---

<div class="tag">anira · Waiting</div>

## Waiting, on both sides.<br><span class="then">Each side has a policy.</span>

<div class="waits">
  <div class="wait">
    <div class="wait-title">the pool, for work</div>
    <div class="wait-row"><code>SPIN_BACKOFF</code><div class="wait-bar spin"><i></i><i></i><i></i><i></i><i></i><i></i><i></i><i></i></div><small>try, pause, yield, sleep</small></div>
    <div class="wait-row"><code>BLOCKING</code><div class="wait-bar block"><i></i></div><small>parked on a semaphore</small></div>
  </div>
  <div class="wait">
    <div class="wait-title">the audio thread, for a result</div>
    <div class="wait-row"><code>wait_ratio = 0</code><div class="wait-bar none"><i></i></div><small>take what is done, never wait</small></div>
    <div class="wait-row"><code>wait_ratio = 0.5</code><div class="wait-bar ratio"><i></i></div><small>half a block period, at most; only in <code>process_wait</code></small></div>
  </div>
</div>

Note:
    - The pool: SPIN_BACKOFF (default) tries a few times, then pauses the core, then yields, then sleeps 100 µs. Exponential backoff, Timur Doumler's recipe from ADC20. BLOCKING parks the thread on a semaphore and the enqueue wakes it.
    - The audio thread: by default it never waits; process() collects what is complete, and a block that is not back yet is handled by the miss policy (next slide).
    - wait_ratio: a bounded wait inside the callback, a fraction of the block period. Only the _wait entries do it; process itself never waits, as in step 6. It buys latency with callback time. Off by default; useful for one instance on a quiet machine, dangerous for ten.

---

<div class="tag">Part 1 · Step 7</div>

## A late block went out as silence.<br><span class="then">We dropped it on arrival, by hand.</span>

```cpp
// A block that came too late already went out as silence. Drop it when
// it arrives, so the wet path stays exactly latency_samples() late.
while (m_wet_debt > 0 && m_output.available() > 0) { … }
```

Note:
    - The wet debt from step 7: our miss policy, written by hand, and it took a bug fix to get right.

---

<div class="tag">anira · Misses</div>

## What a missed block delivers<br><span class="then">is the contract's choice.</span>

```json
{ "hard": { "budget": { "ms": 8.2 }, "warmup": "until_stable", "on_miss": "zeros" } }
```

<div class="statement-points">
  <div><code>bypass</code>: the input block, as it came in</div>
  <div><code>hold_last</code>: the last block the model delivered</div>
  <div><code>zeros</code>: silence</div>
  <div><code>callback</code>: your function, on the audio thread, for that block</div>
</div>

Note:
    - process() returns ANIRA_MISSED for such a block, a success with a delivered count: the buffers hold what the policy chose. The stream stays aligned, as in step 7.
    - The budget is the worst-case inference time the contract promises. Measured during warm-up by default, or explicit: the number from step 3.
    - A miss is not the end: the pool catches up, and the next block is on time again.

---

<div class="tag">Part 1 · Step 7</div>

## One latency for every block size.<br><span class="then">Two blocks, plus the model.</span>

<div class="lat-sum">
  <div class="lat-part">collect a block<strong>2048</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">hand it over<strong>2048</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">the model itself<strong>2224</strong></div>
  <div class="lat-plus">=</div>
  <div class="lat-part total">latency<strong>6320</strong></div>
</div>

Note:
    - Step 7's number: a full block to collect, a full block for the worker, the model's own delay. Safe at any host block size, and generous: at 512 samples per callback, most of the first 2048 is waiting for nothing.

---

<div class="tag">anira · Latency</div>

## Computed at <code>prepare</code>,<br><span class="then">for the block size the host uses.</span>

{{STREAM:host=512|model=2048|length=4096|seconds=5}}

<div class="lat-sum">
  <div class="lat-part">block adaptation<small>P − gcd(b, P)</small><strong>1536</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">inference<small>⌈budget ÷ host block⌉ host blocks</small><strong>512</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">the model itself<strong>2224</strong></div>
  <div class="lat-plus">=</div>
  <div class="lat-part total">latency<strong>4272</strong></div>
</div>

<div class="lat-why">6320 by hand: we gave the worker a whole model block. anira gives it ⌈budget⌉ host blocks: 8.2 ms fit in one of 11.6 ms.</div>

Note:
    - Host block b = 512, model block P = 2048, budget 8.2 ms, 44.1 kHz.
    - Adaptation: P − gcd(b, P) = 1536 samples. The minimum any block-size adapter needs (Rath and Geier, LAC 2026): the model cannot run before its block is complete, and the last host block that completes it is the one that waits least.
    - Inference: 8.2 ms is 0.18 model blocks of audio, 0.7 host blocks. One host block of waiting is enough: 512 samples. With wait_ratio β the callback may absorb part of it.
    - Plus the model's own 2224, declared in the model file as "latency": 2224 on the output.
    - 4272 against our 6320: the same guarantee, 2048 samples less. At b = 441: gcd is 1, adaptation 2047, inference 441, total 4712.
    - The formula also says when it cannot work: if the budget exceeds the audio per hop times the threads, the queue grows without bound, and prepare warns.

---

<div class="tag">Take away</div>

## The pool is shared.<br><span class="then">The policies are yours.</span>

<div class="statement-points">
  <div>Threads: one pool per process, the count set once</div>
  <div>Waiting: a strategy for the pool, a ratio for the callback, a policy for a miss</div>
  <div>Latency: computed from block size, budget and the model, at <code>prepare</code></div>
</div>

Note:
    - Next: the models and the engines, the pre- and post-processing, and where the state lives.
