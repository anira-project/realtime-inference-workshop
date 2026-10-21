<h1>How anira works</h1>

Three configs, then the design

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - From what anira is to how it works: what you declare, then an overview of the next three chapters.

---

<div class="tag">How you use it</div>

## Three configs say what, how, and with what.<br><span class="then">The handler runs it.</span>

<div class="cards">
  <div class="card">{{ICON:model}}<div class="card-title">ModelConfig</div><div class="card-text">what: the files, every tensor, its role</div><div class="card-file">model.json</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:sliders}}<div class="card-title">Contract</div><div class="card-text">how: <em>Hard</em> for a stream, <em>Async</em> for jobs</div><div class="card-file">contract.json</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:chip}}<div class="card-title">ContextConfig</div><div class="card-text">with what: threads, engines, log</div><div class="card-file optional">context.json · optional</div></div>
  <div class="card-arrow">→</div>
  <div class="card accent">{{ICON:shield}}<div class="card-title">Handler</div><div class="card-text">three calls</div><div class="card-file">create · prepare · process</div></div>
</div>

<div class="cards-note">Three JSON files, the context one optional, and three calls. For most models that is all.</div>

<div class="cards-note fragment">Some models need their own pre- and post-processing: a stage, in code. That comes later.</div>

Note:
    - ModelConfig: which files, which engines may run them, every tensor with a name and a role (streamed, static, state).
    - Contract: what the host promises and what it gets. Hard: a block size range, a time budget, what happens on a miss; the latency follows from it. Async: a job with a deadline, for things like a camera frame. Everything in the hands-on is Hard, except the hand tracking in step 11.
    - Version: the workshop pins a v3 pre-release commit (after v3.0.0-alpha.1); CMake fetches it with ONNX Runtime built in. The proper v3.0 release follows at the end of November; details may still move until then.
    - ContextConfig: the process-wide part, threads and enabled engines. One per process, like the pool. It can be a JSON file like the other two; its defaults fit most hosts, so the exercises create it in code.
    - Handler: create once, prepare when the host announces its block size (get_latency then gives the number for setLatencySamples), process in every callback. The places from step 8.
    - Click: the default stage takes a window out of the ring and writes the result back. A model that needs another shape, or its own pre- and post-processing, gets a stage in code; that is in Models and engines.

---

<div class="tag">In the following</div>

## How anira is designed.<br><span class="then">Three chapters, each from part 1 to anira.</span>

<div class="cards">
  <div class="card">{{ICON:threads}}<div class="card-title"><a href="#/threads-and-latency">Threads and latency</a></div><div class="card-text">one pool, no waiting, one number</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:model}}<div class="card-title"><a href="#/models-and-engines">Models and engines</a></div><div class="card-text">plans, stages, state</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:slides}}<div class="card-title"><a href="#/architecture">The whole picture</a></div><div class="card-text">where the pieces sit</div></div>
</div>

Note:
    - Threading and latency first, then models and engines, then all of it in one diagram. Each concept starts from the step in part 1 that built it by hand.
