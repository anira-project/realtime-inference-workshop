<h1>Threads and latency</h1>

One pool, no waiting, one number

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - First the two kinds of thread, then three concepts, each in a pair of slides: the step in part 1 that built it by hand, then what anira does instead.

---

<div class="tag">Two threads</div>

## Two kinds of thread.<br><span class="then">Each has its own rules.</span>

<div class="kinds">
  <div class="kind audio">
    <div class="kind-name">Audio thread</div>
    <div class="kind-role"><code>process</code>, with pre- and post-processing</div>
    <div class="kind-track"><div class="kind-row callbacks"></div></div>
    <div class="kind-axis"><span>one callback</span><span>time →</span></div>
    <div class="kind-rules"><b>no allocation</b><b>no lock</b><b>never waits</b></div>
  </div>
  <div class="kind inference">
    <div class="kind-name">Inference threads</div>
    <div class="kind-role">the model runs: the heavy, not real-time-safe work</div>
    <div class="kind-track">
      <div class="kind-row"><i style="left:2%;width:46%"></i><i style="left:52%;width:40%"></i></div>
      <div class="kind-row"><i style="left:14%;width:52%"></i><i style="left:70%;width:26%"></i></div>
      <div class="kind-row"><i style="left:6%;width:30%"></i><i style="left:40%;width:48%"></i></div>
    </div>
    <div class="kind-axis"><span>one model run</span><span>time →</span></div>
    <div class="kind-rules"><b>may allocate, may lock</b><b>as long as it takes</b><b>shared by every instance</b></div>
  </div>
</div>

Note:
    - The map for this chapter: which thread does what. The pairs that follow fill it in.
    - Left: the audio thread. Every callback runs process with the pre- and post-processing around it, and returns long before its deadline; it never allocates, locks or waits. That is part 1's real-time safety, kept.
    - Right: the inference threads, one pool per process. Only what is not real-time safe or too heavy moves here: the model run, inside an engine that may allocate and lock, for as long as it takes. Every instance in the process shares the pool.
    - Configs, prepare and the latency are set up wherever the host calls them, usually its main thread. That is setup before the stream starts, not a thread anira adds.

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
  <div class="pool-queue"><span>one queue</span><div class="q small"><div class="q-slot"><b></b></div><div class="q-slot"><b></b></div><div class="q-slot"><b></b></div><div class="q-slot"><b></b></div></div></div>
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
    - One lock-free queue (moodycamel) for every handler. The audio thread enqueues a job and leaves; a free thread takes the oldest one. The animation shows it: instances flash when they enqueue, the threads take turns, each busy while its model runs.
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

## The pool waits for work.<br><span class="then">Spin with backoff, or block.</span>

<div class="policies">
  <div class="policy-head"></div><div class="policy-head"></div><div class="policy-head">audio thread, on enqueue</div><div class="policy-head">pool, while idle</div>

  <div class="policy-name"><code>SPIN_BACKOFF</code></div>
  <div class="wait-bar spin"><i></i><i></i><i></i><i></i><i></i><i></i><i></i><i></i></div>
  <div class="policy-cell ok">an atomic, nothing else</div>
  <div class="policy-cell warn">polls: spin, yield, sleep 100 µs</div>

  <div class="policy-name"><code>BLOCKING</code></div>
  <div class="wait-bar block"><i></i></div>
  <div class="policy-cell warn">an atomic, plus one wake syscall when a thread sleeps</div>
  <div class="policy-cell ok">no CPU, woken at once</div>
</div>

<div class="policy-advice fragment"><span>We recommend BLOCKING.</span> One bounded wake instead of threads polling while idle. It takes no lock and never waits, but no OS documents it as real-time safe, and RTSan does not check it.</div>

Note:
    - An idle pool thread has to wait for the next job somehow. SPIN_BACKOFF (default) tries a few times, then pauses the core, then yields, then sleeps 100 µs: exponential backoff, Timur Doumler's recipe from ADC20. Fast to react, costs a little CPU while idle.
    - BLOCKING parks the thread on a semaphore and the enqueue wakes it. No CPU while idle, a little more time to wake up.
    - The choice is the context's, once per process: context.threads(4, ANIRA_WAIT_SPIN_BACKOFF).
    - The cost lands on different threads. With SPIN_BACKOFF the audio thread only does an atomic enqueue; the idle pool pays by polling. With BLOCKING the idle pool costs nothing, but when a pool thread is asleep the enqueue on the audio thread wakes it: sem_post (futex) on Linux and Android, semaphore_signal (a Mach trap) on Apple, ReleaseSemaphore on Windows. Under load the threads rarely sleep and the wake is rare; on an idle pool it happens for nearly every job.
    - Click: our recommendation is BLOCKING. The wake is bounded and never waits, unlike a lock. But no platform documents it as real-time safe, and RTSan intercepts pthread_mutex_unlock and pthread_cond_signal, not sem_post or semaphore_signal, so it will not tell you. We have not measured its worst case per platform yet.

---

<div class="tag">anira · Pipeline</div>

## Audio in, through the pool, audio out.<br><span class="then">Push, then pop, every callback.</span>

<div class="pipeline" data-window="4" data-inference="5.5" data-threads="2" data-slots="4" data-ring="8" data-period="1400" data-loop="40">
  <div class="pl-in"><div class="pl-label">audio in</div><div class="pl-audio"><svg class="pl-wave" viewBox="0 0 120 40" aria-hidden="true"><path class="wave" d="M2 20 C 10 4, 18 4, 26 20 S 42 36, 50 20 S 66 4, 74 20 S 90 36, 98 20 S 114 4, 118 20"/><path class="flat" d="M2 20 L118 20"/></svg></div></div>
  <div class="pl-arrow a1 fragment" data-fragment-index="1" data-stage="1"><span class="pl-push">push</span></div>
  <div class="pl-ring in fragment" data-fragment-index="1" data-stage="1"><div class="pl-label">ring buffer</div><div class="pl-circle"></div></div>
  <div class="pl-arrow a2 fragment" data-fragment-index="2" data-stage="2"><span class="pl-enqueue">enqueue</span></div>
  <div class="pl-slots fragment" data-fragment-index="2" data-stage="2"><div class="pl-label">slots</div><div class="pl-list"></div><div class="pl-legend"><span class="queued">waiting</span><span class="running">running</span><span class="done">done</span></div></div>
  <div class="pl-arrow a3 both fragment" data-fragment-index="3" data-stage="3"></div>
  <div class="pl-threads fragment" data-fragment-index="3" data-stage="3"><div class="pl-label">inference threads</div><div class="pl-list"></div></div>
  <div class="pl-out fragment" data-fragment-index="5" data-stage="5"><div class="pl-label">audio out</div><div class="pl-audio"><svg class="pl-wave" viewBox="0 0 120 40" aria-hidden="true"><path class="wave" d="M2 20 C 10 4, 18 4, 26 20 S 42 36, 50 20 S 66 4, 74 20 S 90 36, 98 20 S 114 4, 118 20"/><path class="flat" d="M2 20 L118 20"/></svg></div></div>
  <div class="pl-arrow a4 back fragment" data-fragment-index="5" data-stage="5"><span class="pl-pop">pop</span></div>
  <div class="pl-ring out fragment" data-fragment-index="4" data-stage="4"><div class="pl-label">ring buffer</div><div class="pl-circle"></div></div>
  <div class="pl-arrow a5 back fragment" data-fragment-index="4" data-stage="4"><span class="pl-dequeue">dequeue</span></div>
  <div class="pl-down fragment" data-fragment-index="4" data-stage="4"><span class="pl-then"></span></div>
</div>

<div class="cards-note fragment" data-fragment-index="5">At the start, silence goes out until the first result is back: that is the latency.</div>

Note:
    - Built up click by click while it runs. Simplified, not to scale.
    - First only the audio coming in: a block per callback, at a steady rate.
    - Click: the push. The audio thread writes every block into the ring buffer, and with nothing taking it out, the ring fills up.
    - Click: the enqueue. Whenever the ring holds enough for one model run, that part goes into a free slot: at a different rate than the blocks. With no one running them, the slots fill up.
    - Click: the inference threads. A free thread takes the oldest waiting slot, runs the model and writes the result back. One run takes longer than the time between two windows here, so the two threads take turns.
    - Click: the dequeue. The audio thread's path in one callback: push, enqueue (only when the ring holds enough), straight down, dequeue. The inference threads on the right run on their own. With wait_ratio = 0 the dequeue comes right after the enqueue: the slot just handed over cannot be back yet, so the dequeue takes an older one, or nothing.
    - Click: the pop. Once a slot is done, the dequeue moves its result into the output ring buffer, and every pop plays one block out of it. Before that, silence goes out.
    - The silence at the start is the latency. anira computes it at prepare and reports it to the host.

---

<div class="tag">anira · Waiting</div>

## Between enqueue and dequeue,<br><span class="then">the audio thread may wait.</span>

<div class="pl-case"><code>wait_ratio = 0</code> the dequeue comes right after the enqueue.</div>

<div class="pipeline" data-window="2" data-inference="0.35" data-wait="0" data-period="2400" data-threads="2" data-slots="4" data-ring="8" data-loop="12">
  <div class="pl-in"><div class="pl-label">audio in</div><div class="pl-audio"><svg class="pl-wave" viewBox="0 0 120 40" aria-hidden="true"><path class="wave" d="M2 20 C 10 4, 18 4, 26 20 S 42 36, 50 20 S 66 4, 74 20 S 90 36, 98 20 S 114 4, 118 20"/><path class="flat" d="M2 20 L118 20"/></svg></div></div>
  <div class="pl-arrow a1"><span class="pl-push">push</span></div>
  <div class="pl-ring in"><div class="pl-label">ring buffer</div><div class="pl-circle"></div></div>
  <div class="pl-arrow a2"><span class="pl-enqueue">enqueue</span></div>
  <div class="pl-slots"><div class="pl-label">slots</div><div class="pl-list"></div><div class="pl-legend"><span class="queued">waiting</span><span class="running">running</span><span class="done">done</span></div></div>
  <div class="pl-arrow a3 both"></div>
  <div class="pl-threads"><div class="pl-label">inference threads</div><div class="pl-list"></div></div>
  <div class="pl-out"><div class="pl-label">audio out</div><div class="pl-audio"><svg class="pl-wave" viewBox="0 0 120 40" aria-hidden="true"><path class="wave" d="M2 20 C 10 4, 18 4, 26 20 S 42 36, 50 20 S 66 4, 74 20 S 90 36, 98 20 S 114 4, 118 20"/><path class="flat" d="M2 20 L118 20"/></svg></div></div>
  <div class="pl-arrow a4 back"><span class="pl-pop">pop</span></div>
  <div class="pl-ring out"><div class="pl-label">ring buffer</div><div class="pl-circle"></div></div>
  <div class="pl-arrow a5 back"><span class="pl-dequeue">dequeue</span></div>
  <div class="pl-down"><span class="pl-then"></span></div>
</div>

Note:
    - The same pipeline with a fast model: a window of two blocks, one run takes about a third of a block. Simplified, not to scale.
    - wait_ratio = 0, the default: the audio thread goes straight from the enqueue to the dequeue. The window it just handed over is still running, so its result comes out one callback later. That callback is latency.

---

<div class="tag">anira · Waiting</div>

## Between enqueue and dequeue,<br><span class="then">the audio thread may wait.</span>

<div class="pl-case"><code>wait_ratio = 0.25</code> the audio thread waits a quarter block: not long enough.</div>

<div class="pipeline" data-window="2" data-inference="0.35" data-wait="0.25" data-period="2400" data-threads="2" data-slots="4" data-ring="8" data-loop="12">
  <div class="pl-in"><div class="pl-label">audio in</div><div class="pl-audio"><svg class="pl-wave" viewBox="0 0 120 40" aria-hidden="true"><path class="wave" d="M2 20 C 10 4, 18 4, 26 20 S 42 36, 50 20 S 66 4, 74 20 S 90 36, 98 20 S 114 4, 118 20"/><path class="flat" d="M2 20 L118 20"/></svg></div></div>
  <div class="pl-arrow a1"><span class="pl-push">push</span></div>
  <div class="pl-ring in"><div class="pl-label">ring buffer</div><div class="pl-circle"></div></div>
  <div class="pl-arrow a2"><span class="pl-enqueue">enqueue</span></div>
  <div class="pl-slots"><div class="pl-label">slots</div><div class="pl-list"></div><div class="pl-legend"><span class="queued">waiting</span><span class="running">running</span><span class="done">done</span></div></div>
  <div class="pl-arrow a3 both"></div>
  <div class="pl-threads"><div class="pl-label">inference threads</div><div class="pl-list"></div></div>
  <div class="pl-out"><div class="pl-label">audio out</div><div class="pl-audio"><svg class="pl-wave" viewBox="0 0 120 40" aria-hidden="true"><path class="wave" d="M2 20 C 10 4, 18 4, 26 20 S 42 36, 50 20 S 66 4, 74 20 S 90 36, 98 20 S 114 4, 118 20"/><path class="flat" d="M2 20 L118 20"/></svg></div></div>
  <div class="pl-arrow a4 back"><span class="pl-pop">pop</span></div>
  <div class="pl-ring out"><div class="pl-label">ring buffer</div><div class="pl-circle"></div></div>
  <div class="pl-arrow a5 back"><span class="pl-dequeue">dequeue</span></div>
  <div class="pl-down"><span class="pl-then"></span><div class="pl-wait" style="--share:0.25"><i class="pl-wait-fill"></i></div></div>
</div>

Note:
    - wait_ratio = 0.25: between enqueue and dequeue the audio thread blocks for up to a quarter of the block period. The arrow down stands for one block period; the dashed box beside it is the share the ratio allows. The model needs a third of a block, so the wait runs out first.
    - The dequeue finds nothing new, the result comes out one callback later as before, and the quarter block of waiting is gone from the callback for nothing.

---

<div class="tag">anira · Waiting</div>

## Between enqueue and dequeue,<br><span class="then">the audio thread may wait.</span>

<div class="pl-case"><code>wait_ratio = 0.5</code> the audio thread waits up to half a block: the result is back in time.</div>

<div class="pipeline" data-window="2" data-inference="0.35" data-wait="0.5" data-period="2400" data-threads="2" data-slots="4" data-ring="8" data-loop="12">
  <div class="pl-in"><div class="pl-label">audio in</div><div class="pl-audio"><svg class="pl-wave" viewBox="0 0 120 40" aria-hidden="true"><path class="wave" d="M2 20 C 10 4, 18 4, 26 20 S 42 36, 50 20 S 66 4, 74 20 S 90 36, 98 20 S 114 4, 118 20"/><path class="flat" d="M2 20 L118 20"/></svg></div></div>
  <div class="pl-arrow a1"><span class="pl-push">push</span></div>
  <div class="pl-ring in"><div class="pl-label">ring buffer</div><div class="pl-circle"></div></div>
  <div class="pl-arrow a2"><span class="pl-enqueue">enqueue</span></div>
  <div class="pl-slots"><div class="pl-label">slots</div><div class="pl-list"></div><div class="pl-legend"><span class="queued">waiting</span><span class="running">running</span><span class="done">done</span></div></div>
  <div class="pl-arrow a3 both"></div>
  <div class="pl-threads"><div class="pl-label">inference threads</div><div class="pl-list"></div></div>
  <div class="pl-out"><div class="pl-label">audio out</div><div class="pl-audio"><svg class="pl-wave" viewBox="0 0 120 40" aria-hidden="true"><path class="wave" d="M2 20 C 10 4, 18 4, 26 20 S 42 36, 50 20 S 66 4, 74 20 S 90 36, 98 20 S 114 4, 118 20"/><path class="flat" d="M2 20 L118 20"/></svg></div></div>
  <div class="pl-arrow a4 back"><span class="pl-pop">pop</span></div>
  <div class="pl-ring out"><div class="pl-label">ring buffer</div><div class="pl-circle"></div></div>
  <div class="pl-arrow a5 back"><span class="pl-dequeue">dequeue</span></div>
  <div class="pl-down"><span class="pl-then"></span><div class="pl-wait" style="--share:0.5"><i class="pl-wait-fill"></i></div></div>
</div>

Note:
    - wait_ratio = 0.5: the audio thread blocks for up to half a block. The model is done after a third, the audio thread wakes up, and the dequeue takes this window's result right away.
    - The result goes out in the same callback: the silence at the start is shorter, the latency is one block lower.
    - It buys latency with callback time: fine for one instance on a quiet machine, dangerous for ten. Only the _wait entries wait; process itself never does.

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
