<h1>Process</h1>

One block through anira

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - The third row of the map: every callback. The two kinds of thread, the path of one block, the stages, the waiting, a miss, and the real-time rules.

---

<div class="tag">The map</div>

## Process runs in every callback,<br><span class="then">on the audio thread.</span>

{{MAP:show=create,prepare|focus=process}}

Note:
    - The row we fill now: processBlock. One call per callback; everything else in the row is anira's, on the audio thread and in the pool.

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
    - The two halves of the process row: the audio thread on the left, the pool on the right. The slides that follow fill them in.
    - Left: the audio thread. Every callback runs process with the pre- and post-processing around it, and returns long before its deadline; it never allocates, locks or waits. That is part 1's real-time safety, kept.
    - Right: the inference threads, one pool per process. Only what is not real-time safe or too heavy moves here: the model run, inside an engine that may allocate and lock, for as long as it takes. Every instance in the process shares the pool.
    - Configs, prepare and the latency are set up wherever the host calls them, usually its main thread. That is setup before the stream starts, not a thread anira adds.

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

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 2 and 6</div>

## Pre- and post-processing,<br><span class="then">inside the processor, by hand.</span>

```cpp
m_input.push(samples, num_samples);                     // host block in
while (m_input.available() >= k_model.m_input_size) {   // model block out
    … try_enqueue …
}
…
m_output.pop(m_wet_block.data(), num_samples);          // host block out
```

Note:
    - Ring in, model block out, model block in, ring out: that is pre- and post-processing, and ours knew only one shape, audio to audio.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Stages</div>

## Four phases around the engine.<br><span class="then">Two threads, one state loop.</span>

<svg class="st" viewBox="0 0 1600 560" aria-label="the stage phases">
  <defs>
    <marker id="st-arrow" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="5" markerHeight="5" orient="auto-start-reverse">
      <path d="M0 0 L10 5 L0 10z" class="ov-arrowhead"/>
    </marker>
  </defs>
  <rect class="st-lane audio" x="20" y="40" width="1570" height="200" rx="10"/>
  <text x="40" y="70" class="st-lane-label">audio thread · driver-thread</text>
  <rect class="st-lane pool" x="20" y="300" width="1570" height="200" rx="10"/>
  <text x="40" y="330" class="st-lane-label">inference thread · one of the pool</text>
  <g class="fragment" data-fragment-index="1">
    <rect class="ov-pill" x="50" y="112" width="130" height="56" rx="8"/><text x="115" y="146" class="st-text">host block</text>
    <line class="ov-flow strong" x1="180" y1="140" x2="226" y2="140" marker-end="url(#st-arrow)"/>
    <circle class="ov-ring" cx="250" cy="140" r="24"/><text x="250" y="194" class="st-caption">input ring</text>
    <line class="ov-flow strong" x1="274" y1="140" x2="300" y2="140" marker-end="url(#st-arrow)"/>
    <rect class="ov-pill stage" x="300" y="112" width="170" height="56" rx="8"/><text x="385" y="146" class="st-text">pre_process</text>
    <text x="385" y="194" class="st-caption">a window out of the ring</text>
    <line class="ov-flow strong" x1="470" y1="140" x2="510" y2="140" marker-end="url(#st-arrow)"/>
    <rect class="ov-pill tensor" x="510" y="112" width="180" height="56" rx="8"/><text x="600" y="146" class="st-text">model tensor in</text>
    <text x="600" y="194" class="st-caption">a slot, stamped</text>
  </g>
  <g class="fragment" data-fragment-index="2">
    <path class="ov-flow strong" d="M600 168 V380" marker-end="url(#st-arrow)"/>
    <text x="616" y="262" class="st-caption left">the queue</text>
    <rect class="ov-pill stage" x="505" y="380" width="190" height="56" rx="8"/><text x="600" y="414" class="st-text">before_inference</text>
    <line class="ov-flow strong" x1="695" y1="408" x2="745" y2="408" marker-end="url(#st-arrow)"/>
    <rect class="ov-pill engine" x="745" y="372" width="150" height="72" rx="10"/><text x="820" y="414" class="st-text strong">engine</text>
    <line class="ov-flow strong" x1="895" y1="408" x2="945" y2="408" marker-end="url(#st-arrow)"/>
    <rect class="ov-pill stage" x="945" y="380" width="190" height="56" rx="8"/><text x="1040" y="414" class="st-text">after_inference</text>
    <path class="ov-flow strong" d="M1040 380 V168" marker-end="url(#st-arrow)"/>
    <text x="1056" y="262" class="st-caption left">the slot, back</text>
    <rect class="ov-pill tensor" x="945" y="112" width="190" height="56" rx="8"/><text x="1040" y="146" class="st-text">model tensor out</text>
    <text x="1040" y="194" class="st-caption">the next stamp</text>
    <line class="ov-flow strong" x1="1135" y1="140" x2="1180" y2="140" marker-end="url(#st-arrow)"/>
    <rect class="ov-pill stage" x="1180" y="112" width="170" height="56" rx="8"/><text x="1265" y="146" class="st-text">post_process</text>
    <text x="1265" y="194" class="st-caption">the result into the ring</text>
    <line class="ov-flow strong" x1="1350" y1="140" x2="1376" y2="140" marker-end="url(#st-arrow)"/>
    <circle class="ov-ring" cx="1400" cy="140" r="24"/><text x="1400" y="194" class="st-caption">output ring</text>
    <line class="ov-flow strong" x1="1424" y1="140" x2="1450" y2="140" marker-end="url(#st-arrow)"/>
    <rect class="ov-pill" x="1450" y="112" width="130" height="56" rx="8"/><text x="1515" y="146" class="st-text">host block</text>
  </g>
  <g class="fragment" data-fragment-index="3">
    <path class="ov-flow state" d="M880 444 V482 H760 V444" marker-end="url(#st-arrow)"/>
    <text x="820" y="470" class="st-caption">state_out → state_in</text>
    <text x="820" y="530" class="st-caption">the declared pair swaps around the engine, no copy</text>
  </g>
</svg>

<div class="cards-note">The default stage is step 2: a window out of the ring, the result back in. <code>before</code> and <code>after</code> are empty. You write a stage for another shape.</div>

Note:
    - A stage is the 3.x form of the PrePostProcessor: one per pipeline, with up to four phases. The default stage does what step 2 did; a receptive-field model declares window and overlap, and the default stage fills the history.
    - Top lane, the audio thread: the host block goes into the input ring; pre_process takes one window out of it into the model tensor; the tensor sits in a slot, stamped, and goes into the queue. The callback returns.
    - Bottom lane, a pool thread: before_inference, the engine, after_inference, on the model tensors in place. Then the slot goes back.
    - Top lane again, a later callback: the oldest finished stamp is taken, post_process pushes the result into the output ring, and the host block is served from it.
    - The state loop sits around the engine itself, not around the stage: anira binds the declared pair to two buffers and swaps them per inference. What came out is what goes in next. No copy, and no code of yours.
    - Four phases, at most one stage per pipeline. pre and post are nonblocking: a stage that allocates there is your bug, and RTSan sees it.

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

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 6</div>

## What happens if an inference is too late?<br><span class="then">In step 6 the block went out as zeros.</span>

<div class="click">
  <div class="click-labels"><span style="left:50.00%;width:12.50%">late block: zeros</span></div>
  <svg class="click-wave" viewBox="0 0 1500 260" aria-hidden="true">
  <rect class="click-gap" x="750" y="0" width="188" height="260"/>
  <line class="click-div" x1="188" y1="8" x2="188" y2="252"/><line class="click-div" x1="375" y1="8" x2="375" y2="252"/><line class="click-div" x1="562" y1="8" x2="562" y2="252"/><line class="click-div" x1="750" y1="8" x2="750" y2="252"/><line class="click-div" x1="938" y1="8" x2="938" y2="252"/><line class="click-div" x1="1125" y1="8" x2="1125" y2="252"/><line class="click-div" x1="1312" y1="8" x2="1312" y2="252"/>
  <path class="click-line" d="M0.0 25.0 L2.5 27.3 L5.0 34.1 L7.5 45.1 L10.0 59.7 L12.5 77.5 L15.0 97.6 L17.5 119.0 L20.0 141.0 L22.5 162.4 L25.0 182.5 L27.5 200.3 L30.0 214.9 L32.5 225.9 L35.0 232.7 L37.5 235.0 L40.0 232.7 L42.5 225.9 L45.0 214.9 L47.5 200.3 L50.0 182.5 L52.5 162.4 L55.0 141.0 L57.5 119.0 L60.0 97.6 L62.5 77.5 L65.0 59.7 L67.5 45.1 L70.0 34.1 L72.5 27.3 L75.0 25.0 L77.5 27.3 L80.0 34.1 L82.5 45.1 L85.0 59.7 L87.5 77.5 L90.0 97.6 L92.5 119.0 L95.0 141.0 L97.5 162.4 L100.0 182.5 L102.5 200.3 L105.0 214.9 L107.5 225.9 L110.0 232.7 L112.5 235.0 L115.0 232.7 L117.5 225.9 L120.0 214.9 L122.5 200.3 L125.0 182.5 L127.5 162.4 L130.0 141.0 L132.5 119.0 L135.0 97.6 L137.5 77.5 L140.0 59.7 L142.5 45.1 L145.0 34.1 L147.5 27.3 L150.0 25.0 L152.5 27.3 L155.0 34.1 L157.5 45.1 L160.0 59.7 L162.5 77.5 L165.0 97.6 L167.5 119.0 L170.0 141.0 L172.5 162.4 L175.0 182.5 L177.5 200.3 L180.0 214.9 L182.5 225.9 L185.0 232.7 L187.5 235.0 L190.0 232.7 L192.5 225.9 L195.0 214.9 L197.5 200.3 L200.0 182.5 L202.5 162.4 L205.0 141.0 L207.5 119.0 L210.0 97.6 L212.5 77.5 L215.0 59.7 L217.5 45.1 L220.0 34.1 L222.5 27.3 L225.0 25.0 L227.5 27.3 L230.0 34.1 L232.5 45.1 L235.0 59.7 L237.5 77.5 L240.0 97.6 L242.5 119.0 L245.0 141.0 L247.5 162.4 L250.0 182.5 L252.5 200.3 L255.0 214.9 L257.5 225.9 L260.0 232.7 L262.5 235.0 L265.0 232.7 L267.5 225.9 L270.0 214.9 L272.5 200.3 L275.0 182.5 L277.5 162.4 L280.0 141.0 L282.5 119.0 L285.0 97.6 L287.5 77.5 L290.0 59.7 L292.5 45.1 L295.0 34.1 L297.5 27.3 L300.0 25.0 L302.5 27.3 L305.0 34.1 L307.5 45.1 L310.0 59.7 L312.5 77.5 L315.0 97.6 L317.5 119.0 L320.0 141.0 L322.5 162.4 L325.0 182.5 L327.5 200.3 L330.0 214.9 L332.5 225.9 L335.0 232.7 L337.5 235.0 L340.0 232.7 L342.5 225.9 L345.0 214.9 L347.5 200.3 L350.0 182.5 L352.5 162.4 L355.0 141.0 L357.5 119.0 L360.0 97.6 L362.5 77.5 L365.0 59.7 L367.5 45.1 L370.0 34.1 L372.5 27.3 L375.0 25.0 L377.5 27.3 L380.0 34.1 L382.5 45.1 L385.0 59.7 L387.5 77.5 L390.0 97.6 L392.5 119.0 L395.0 141.0 L397.5 162.4 L400.0 182.5 L402.5 200.3 L405.0 214.9 L407.5 225.9 L410.0 232.7 L412.5 235.0 L415.0 232.7 L417.5 225.9 L420.0 214.9 L422.5 200.3 L425.0 182.5 L427.5 162.4 L430.0 141.0 L432.5 119.0 L435.0 97.6 L437.5 77.5 L440.0 59.7 L442.5 45.1 L445.0 34.1 L447.5 27.3 L450.0 25.0 L452.5 27.3 L455.0 34.1 L457.5 45.1 L460.0 59.7 L462.5 77.5 L465.0 97.6 L467.5 119.0 L470.0 141.0 L472.5 162.4 L475.0 182.5 L477.5 200.3 L480.0 214.9 L482.5 225.9 L485.0 232.7 L487.5 235.0 L490.0 232.7 L492.5 225.9 L495.0 214.9 L497.5 200.3 L500.0 182.5 L502.5 162.4 L505.0 141.0 L507.5 119.0 L510.0 97.6 L512.5 77.5 L515.0 59.7 L517.5 45.1 L520.0 34.1 L522.5 27.3 L525.0 25.0 L527.5 27.3 L530.0 34.1 L532.5 45.1 L535.0 59.7 L537.5 77.5 L540.0 97.6 L542.5 119.0 L545.0 141.0 L547.5 162.4 L550.0 182.5 L552.5 200.3 L555.0 214.9 L557.5 225.9 L560.0 232.7 L562.5 235.0 L565.0 232.7 L567.5 225.9 L570.0 214.9 L572.5 200.3 L575.0 182.5 L577.5 162.4 L580.0 141.0 L582.5 119.0 L585.0 97.6 L587.5 77.5 L590.0 59.7 L592.5 45.1 L595.0 34.1 L597.5 27.3 L600.0 25.0 L602.5 27.3 L605.0 34.1 L607.5 45.1 L610.0 59.7 L612.5 77.5 L615.0 97.6 L617.5 119.0 L620.0 141.0 L622.5 162.4 L625.0 182.5 L627.5 200.3 L630.0 214.9 L632.5 225.9 L635.0 232.7 L637.5 235.0 L640.0 232.7 L642.5 225.9 L645.0 214.9 L647.5 200.3 L650.0 182.5 L652.5 162.4 L655.0 141.0 L657.5 119.0 L660.0 97.6 L662.5 77.5 L665.0 59.7 L667.5 45.1 L670.0 34.1 L672.5 27.3 L675.0 25.0 L677.5 27.3 L680.0 34.1 L682.5 45.1 L685.0 59.7 L687.5 77.5 L690.0 97.6 L692.5 119.0 L695.0 141.0 L697.5 162.4 L700.0 182.5 L702.5 200.3 L705.0 214.9 L707.5 225.9 L710.0 232.7 L712.5 235.0 L715.0 232.7 L717.5 225.9 L720.0 214.9 L722.5 200.3 L725.0 182.5 L727.5 162.4 L730.0 141.0 L732.5 119.0 L735.0 97.6 L737.5 77.5 L740.0 59.7 L742.5 45.1 L745.0 34.1 L747.5 27.3 L750.0 25.0 L750.0 130.0 L937.5 130.0 L937.5 235.0 L940.0 232.7 L942.5 225.9 L945.0 214.9 L947.5 200.3 L950.0 182.5 L952.5 162.4 L955.0 141.0 L957.5 119.0 L960.0 97.6 L962.5 77.5 L965.0 59.7 L967.5 45.1 L970.0 34.1 L972.5 27.3 L975.0 25.0 L977.5 27.3 L980.0 34.1 L982.5 45.1 L985.0 59.7 L987.5 77.5 L990.0 97.6 L992.5 119.0 L995.0 141.0 L997.5 162.4 L1000.0 182.5 L1002.5 200.3 L1005.0 214.9 L1007.5 225.9 L1010.0 232.7 L1012.5 235.0 L1015.0 232.7 L1017.5 225.9 L1020.0 214.9 L1022.5 200.3 L1025.0 182.5 L1027.5 162.4 L1030.0 141.0 L1032.5 119.0 L1035.0 97.6 L1037.5 77.5 L1040.0 59.7 L1042.5 45.1 L1045.0 34.1 L1047.5 27.3 L1050.0 25.0 L1052.5 27.3 L1055.0 34.1 L1057.5 45.1 L1060.0 59.7 L1062.5 77.5 L1065.0 97.6 L1067.5 119.0 L1070.0 141.0 L1072.5 162.4 L1075.0 182.5 L1077.5 200.3 L1080.0 214.9 L1082.5 225.9 L1085.0 232.7 L1087.5 235.0 L1090.0 232.7 L1092.5 225.9 L1095.0 214.9 L1097.5 200.3 L1100.0 182.5 L1102.5 162.4 L1105.0 141.0 L1107.5 119.0 L1110.0 97.6 L1112.5 77.5 L1115.0 59.7 L1117.5 45.1 L1120.0 34.1 L1122.5 27.3 L1125.0 25.0 L1127.5 27.3 L1130.0 34.1 L1132.5 45.1 L1135.0 59.7 L1137.5 77.5 L1140.0 97.6 L1142.5 119.0 L1145.0 141.0 L1147.5 162.4 L1150.0 182.5 L1152.5 200.3 L1155.0 214.9 L1157.5 225.9 L1160.0 232.7 L1162.5 235.0 L1165.0 232.7 L1167.5 225.9 L1170.0 214.9 L1172.5 200.3 L1175.0 182.5 L1177.5 162.4 L1180.0 141.0 L1182.5 119.0 L1185.0 97.6 L1187.5 77.5 L1190.0 59.7 L1192.5 45.1 L1195.0 34.1 L1197.5 27.3 L1200.0 25.0 L1202.5 27.3 L1205.0 34.1 L1207.5 45.1 L1210.0 59.7 L1212.5 77.5 L1215.0 97.6 L1217.5 119.0 L1220.0 141.0 L1222.5 162.4 L1225.0 182.5 L1227.5 200.3 L1230.0 214.9 L1232.5 225.9 L1235.0 232.7 L1237.5 235.0 L1240.0 232.7 L1242.5 225.9 L1245.0 214.9 L1247.5 200.3 L1250.0 182.5 L1252.5 162.4 L1255.0 141.0 L1257.5 119.0 L1260.0 97.6 L1262.5 77.5 L1265.0 59.7 L1267.5 45.1 L1270.0 34.1 L1272.5 27.3 L1275.0 25.0 L1277.5 27.3 L1280.0 34.1 L1282.5 45.1 L1285.0 59.7 L1287.5 77.5 L1290.0 97.6 L1292.5 119.0 L1295.0 141.0 L1297.5 162.4 L1300.0 182.5 L1302.5 200.3 L1305.0 214.9 L1307.5 225.9 L1310.0 232.7 L1312.5 235.0 L1315.0 232.7 L1317.5 225.9 L1320.0 214.9 L1322.5 200.3 L1325.0 182.5 L1327.5 162.4 L1330.0 141.0 L1332.5 119.0 L1335.0 97.6 L1337.5 77.5 L1340.0 59.7 L1342.5 45.1 L1345.0 34.1 L1347.5 27.3 L1350.0 25.0 L1352.5 27.3 L1355.0 34.1 L1357.5 45.1 L1360.0 59.7 L1362.5 77.5 L1365.0 97.6 L1367.5 119.0 L1370.0 141.0 L1372.5 162.4 L1375.0 182.5 L1377.5 200.3 L1380.0 214.9 L1382.5 225.9 L1385.0 232.7 L1387.5 235.0 L1390.0 232.7 L1392.5 225.9 L1395.0 214.9 L1397.5 200.3 L1400.0 182.5 L1402.5 162.4 L1405.0 141.0 L1407.5 119.0 L1410.0 97.6 L1412.5 77.5 L1415.0 59.7 L1417.5 45.1 L1420.0 34.1 L1422.5 27.3 L1425.0 25.0 L1427.5 27.3 L1430.0 34.1 L1432.5 45.1 L1435.0 59.7 L1437.5 77.5 L1440.0 97.6 L1442.5 119.0 L1445.0 141.0 L1447.5 162.4 L1450.0 182.5 L1452.5 200.3 L1455.0 214.9 L1457.5 225.9 L1460.0 232.7 L1462.5 235.0 L1465.0 232.7 L1467.5 225.9 L1470.0 214.9 L1472.5 200.3 L1475.0 182.5 L1477.5 162.4 L1480.0 141.0 L1482.5 119.0 L1485.0 97.6 L1487.5 77.5 L1490.0 59.7 L1492.5 45.1 L1495.0 34.1 L1497.5 27.3 L1500.0 25.0"/>
  <g class="fragment" data-fragment-index="1">
    <line class="click-jump" x1="750.0" y1="25.0" x2="750.0" y2="130"/>
    <line class="click-jump" x1="937.5" y1="130" x2="937.5" y2="235.0"/>
  </g>
</svg>
  <div class="click-note fragment" data-fragment-index="1"><b>click</b> A sudden drop to zero and back: an audible click.</div>
</div>

Note:
    - Step 6's simple answer to a late inference: the block that is not back in time goes out as zeros.
    - Click: the signal jumps to zero at the start of that block and back at its end. Two discontinuities, an audible click, at every miss.
    - And the late result still arrives afterwards. Step 6 drops it on arrival (m_wet_debt counts the samples that already went out as silence), so the wet path stays exactly latency_samples() late; it took a bug fix to get right.
    - anira makes this a choice of the contract instead of code in your plugin: next slide.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Misses</div>

## What a missed block delivers<br><span class="then">is the contract's choice.</span>

```json
{ "hard": { "budget": { "ms": 8.2 }, "warmup": { "fixed": 3 }, "on_miss": "bypass" } }
```

<div class="statement-points">
  <div><code>bypass</code>, the default: the input block, as it came in</div>
  <div><code>hold_last</code>: the head of the last block the model delivered</div>
  <div><code>zeros</code>: silence</div>
  <div><code>callback</code>: your function, on the audio thread, for that block</div>
</div>

<div class="cards-note">The stream stays aligned: a late result that arrives afterwards is dropped, never played shifted.</div>

Note:
    - process() returns ANIRA_MISSED for such a block, a success: the buffers hold what the policy chose. The stream stays aligned, as in step 6, without the bug fix.
    - Catch-up is strict: after a miss every late sample that arrives is discarded, so a delivered block is the stream at exactly its latency, or the policy's fill.
    - Overload is the same event one level down: when a callback has a window ready and no slot is free, that window is dropped and filled by the policy; rt_error records ANIRA_ERROR_CAPACITY.
    - The same rule bites a pop that asks for more than is there: it is a miss. Step 10 checks get_available_samples before it pops.
    - A miss is not the end: the pool catches up, and the next block is on time again.

---

<div class="tag">anira · Next</div>

## Inline execution.<br><span class="then">No inference term in the latency.</span>

<div class="statement-points">
  <div><code>execution</code> in the contract: <code>worker</code>, <code>inline</code> or <code>auto</code>, per plan, fixed at <code>prepare</code></div>
  <div><code>inline</code> runs the model inside <code>process</code>, on the audio thread</div>
  <div><code>auto</code> picks inline only for an engine that declares itself real-time safe, and only if the measured budget fits</div>
</div>

<div class="cards-note">With alpha.3 (M2b). Until then every inference runs on the pool.</div>

Note:
    - For a model that is small enough, handing it to the pool costs more latency than it saves: the inference term of the formula is at least one host block.
    - Inline removes that term: the model runs in the callback, and inline_ratio bounds the share of the block period it may take. An infeasible inline is ANIRA_ERROR_BUDGET at prepare.
    - It is only safe for an engine without allocations and locks in its run, which is why auto asks the engine's flag. A hand-written network is the typical case.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 4</div>

## We checked real-time safety by hand.<br><span class="then">Once, on our machine.</span>

```cpp
void audio_callback(Engine& engine, float* samples, size_t num_samples)
    [[clang::nonblocking]] { … }
```

<div class="cards-note">RTSan on both engines, one run: what it caught, and what it could not see.</div>

Note:
    - Step 4: the callback tagged nonblocking, the build with -fsanitize=realtime, one run per engine. It has to be repeated for every change, every model, every engine.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Safety</div>

## Every entry names its thread.<br><span class="then">The audio-thread entries are nonblocking.</span>

<div class="threads">
  <div class="threads-label">main</div>
  <div class="threads-lane"><span>from_file</span><span>create</span><span>prepare</span><span>get_latency</span><span>destroy</span></div>
  <div class="threads-label">audio</div>
  <div class="threads-lane audio"><span>process</span><span class="stage">pre_process</span><span class="stage">post_process</span></div>
  <div class="threads-label">pool</div>
  <div class="threads-lane pool"><span class="stage">before_inference</span><span class="engine">engine</span><span class="stage">after_inference</span></div>
</div>

<div class="cards-note">Every audio-thread entry carries <code>ANIRA_NONBLOCKING</code>; the whole test suite runs under RTSan in CI.</div>

Note:
    - The thread tags are part of the API, the way CLAP does it: main-thread for the control path, driver-thread for the host's real-time callback, inference-thread for the pool. Every entry in the headers says which one may call it.
    - ANIRA_NONBLOCKING is clang's nonblocking attribute on every driver-thread entry: the compiler checks what it calls (-Werror=function-effects), and the library's CI runs every test under RTSan.
    - The library logs from the audio thread without a lock: anira_log_rt writes a fixed record into a queue, and a drain thread prints it. A failed audio-thread call says why in rt_error.
    - A custom stage's pre_process and post_process run on the audio thread too: allocating there is your bug, and RTSan sees it.

---

<div class="tag">The map</div>

## After process:<br><span class="then">the whole path of one block.</span>

{{MAP:show=create,prepare,process}}

Note:
    - The third row, filled: input tensor, ring, pre_process, a stamped slot, a pool thread, the engine with the state pair around it, post_process, ring, output tensor.
    - Every row filled. The last chapter looks at the map as one piece.

---

<div class="tag">Take away</div>

## The callback never waits.<br><span class="then">The policies are yours.</span>

<div class="statement-points">
  <div>Waiting: a strategy for the pool, a ratio for the callback, a policy for a miss</div>
  <div>Stages: four phases around the engine, two of them on the audio thread</div>
  <div>Safety: every entry names its thread; the audio-thread ones are checked</div>
</div>

Note:
    - Next: the whole picture, the C ABI underneath, and the three places in a plugin.
