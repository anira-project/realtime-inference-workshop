<h1>anira</h1>

Replacing the hand-built parts

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->
<!-- part: anira -->

Note:
    - Part 2. Everything in part 1 was built so that this part can name it.
    - Talk first: what anira is, then the concepts one by one, then the architecture. Hands-on after that.

---

<div class="tag">Part 1</div>

## Everything we built<br><span class="then">has a name in anira.</span>

<div class="handoff">
  <div class="handoff-head"></div>
  <div class="handoff-head">by hand</div>
  <div class="handoff-head">anira</div>
  <div class="handoff-step">step 2</div><div class="handoff-ours">ring buffers</div><div class="handoff-anira fragment">a ring per streamed tensor</div>
  <div class="handoff-step">step 3</div><div class="handoff-ours">a benchmark</div><div class="handoff-anira fragment">a budget, measured and checked</div>
  <div class="handoff-step">step 4</div><div class="handoff-ours">an engine choice</div><div class="handoff-anira fragment">a plan per engine</div>
  <div class="handoff-step">step 5</div><div class="handoff-ours">RTSan</div><div class="handoff-anira fragment"><code>nonblocking</code> entries, RTSan in CI</div>
  <div class="handoff-step">step 6</div><div class="handoff-ours">a worker, two queues</div><div class="handoff-anira fragment">one pool per process</div>
  <div class="handoff-step">step 7</div><div class="handoff-ours">a prefill rule</div><div class="handoff-anira fragment">an <code>on_miss</code> policy</div>
  <div class="handoff-step">step 7</div><div class="handoff-ours">a latency calculation</div><div class="handoff-anira fragment"><code>get_latency()</code>, closed form</div>
  <div class="handoff-step">step 7</div><div class="handoff-ours">a dry path</div><div class="handoff-anira fragment">still yours, one number to delay it by</div>
  <div class="handoff-step">step 8</div><div class="handoff-ours">one engine per channel</div><div class="handoff-anira fragment">one handler per stream, one pool underneath</div>
</div>

Note:
    - The list from the end of step 8, one more time. Reveal the right column row by row.
    - Every row is one of the concepts in the next two chapters. The dry path is the one thing that stays in your code: anira reports the number, the mix is the plugin's.

---

<div class="tag">Motivation</div>

## One plugin, two threads.<br><span class="then">Ten plugins, twenty threads, eight cores.</span>

<div class="crowd">
  <div class="crowd-plugins">
    <div class="crowd-plugin" style="--i:0"><span>instance 1</span><i></i><i></i></div>
    <div class="crowd-plugin" style="--i:1"><span>instance 2</span><i></i><i></i></div>
    <div class="crowd-plugin" style="--i:2"><span>instance 3</span><i></i><i></i></div>
    <div class="crowd-plugin" style="--i:3"><span>instance 4</span><i></i><i></i></div>
    <div class="crowd-plugin over" style="--i:4"><span>instance 5</span><i></i><i></i></div>
    <div class="crowd-plugin over" style="--i:5"><span>instance 6</span><i></i><i></i></div>
    <div class="crowd-plugin over" style="--i:6"><span>instance 7</span><i></i><i></i></div>
    <div class="crowd-plugin over" style="--i:7"><span>instance 8</span><i></i><i></i></div>
    <div class="crowd-plugin over" style="--i:8"><span>instance 9</span><i></i><i></i></div>
    <div class="crowd-plugin over" style="--i:9"><span>instance 10</span><i></i><i></i></div>
  </div>
  <div class="crowd-cores">
    <span>8 cores</span>
    <b style="--i:0"></b><b style="--i:1"></b><b style="--i:2"></b><b style="--i:3"></b>
    <b style="--i:4"></b><b style="--i:5"></b><b style="--i:6"></b><b style="--i:7"></b>
  </div>
  <div class="crowd-count"><strong>20</strong> real-time threads on <strong>8</strong> cores</div>
</div>

Note:
    - Step 8 ended here: one engine and one worker per channel, per instance. The host does not know, and keeps adding instances.
    - Real-time threads that outnumber cores take turns. A worker that is not scheduled misses its block, and the miss lands in someone else's plugin.
    - This is the problem anira was written for. Everything else followed from it.

---

<div class="tag">History</div>

## Written for that problem,<br><span class="then">in 2024.</span>

<div class="history">
  <div class="history-item fragment"><div class="history-year">2023</div><div class="history-name">ADC23</div><div class="history-text">one inference thread per plugin</div></div>
  <div class="history-item fragment"><div class="history-year">2024</div><div class="history-name">anira · ADC24</div><div class="history-text">one pool per process; IS² paper</div></div>
  <div class="history-item fragment"><div class="history-year">11/2024</div><div class="history-name">v1</div><div class="history-text">context, multichannel</div></div>
  <div class="history-item fragment"><div class="history-year">07/2025</div><div class="history-name">v2</div><div class="history-text">any tensor, not only audio</div></div>
  <div class="history-item fragment"><div class="history-year">2026</div><div class="history-name">v2.1 – v2.3</div><div class="history-text">web, mobile, generators</div></div>
  <div class="history-item fragment"><div class="history-year">2026</div><div class="history-name">v3</div><div class="history-text">one C ABI, every host</div></div>
</div>

<div class="history-note">Apache 2.0 · tanh lab, TU Berlin · funded by BMFTR since 2026</div>

Note:
    - ADC23 (London): the nn-inference-template, an inference thread per plugin, and the oversubscription slide at the end.
    - ADC24 (Bristol): anira, with the static thread pool, and the IEEE IS² 2024 paper, which won the Industry Award.
    - v1: anira::Context, one pool shared by every handler; multichannel. v2: arbitrary tensors, non-audio inputs and outputs, different rates in and out.
    - 2026: web (WASM), Android and iOS, LiteRT and ExecuTorch, generators and analysers, a closed-form latency. Then v3: a versioned C ABI, so every host and language sees the same library.
    - Since 2026 funded by the BMFTR through the Prototype Fund; Lina joined the team.

---

<div class="tag">What anira is</div>

## Three configs say what, how, and with what.<br><span class="then">The handler runs it.</span>

<div class="cards">
  <div class="card">{{ICON:model}}<div class="card-title">ModelConfig</div><div class="card-text">what: the files, every tensor, its role</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:sliders}}<div class="card-title">Contract</div><div class="card-text">how: <em>Hard</em> for a stream, <em>Async</em> for jobs</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:chip}}<div class="card-title">ContextConfig</div><div class="card-text">with what: threads, engines, log</div></div>
  <div class="card-arrow">→</div>
  <div class="card accent">{{ICON:shield}}<div class="card-title">Handler</div><div class="card-text"><code>prepare</code> · <code>process</code> · <code>get_latency</code></div></div>
</div>

<div class="cards-note">Two JSON files and three calls. The rest is the library's. Today: v3.0.0-beta, pinned by CMake.</div>

Note:
    - ModelConfig: which files, which engines may run them, every tensor with a name and a role (streamed, static, state).
    - Contract: what the host promises and what it gets. Hard: a block size range, a time budget, what happens on a miss; the latency follows from it. Async: a job with a deadline, for things like a camera frame. Everything in the hands-on is Hard, except the hand tracking in step 11.
    - Version: the workshop pins the v3 beta tag; CMake fetches it with ONNX Runtime built in.
    - ContextConfig: the process-wide part, threads and enabled engines. One per process, like the pool.
    - Handler: the three calls from step 8, prepareToPlay, processBlock and setLatencySamples, one each.

---

<div class="tag">Take away</div>

## Three kinds of thread.<br><span class="then">Each does one thing.</span>

<div class="statement-points">
  <div>The main thread plans: configs, <code>prepare</code>, the latency</div>
  <div>The audio thread moves samples: <code>process</code>, nothing else</div>
  <div>The pool runs the models</div>
</div>

Note:
    - That is the whole library in one sentence. The next two chapters go through the concepts behind it, each one starting from the step in part 1 that built it by hand.
