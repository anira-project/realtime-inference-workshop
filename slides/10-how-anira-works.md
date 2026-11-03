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
    - Contract: what the host promises and what it gets. Hard: a block size range, a time budget, what happens on a miss; the latency follows from it. Async: a job with a deadline, for things like a camera frame; designed, and built with M3, so alpha.2 refuses it. Everything in the hands-on is Hard, step 11 included.
    - Version: the workshop pins the v3 pre-release v3.0.0-alpha.2; CMake fetches it with ONNX Runtime built in. The proper v3.0 release follows at the end of November; details may still move until then.
    - ContextConfig: the process-wide part, threads and enabled engines. One per process, like the pool. It can be a JSON file like the other two; its defaults fit most hosts, so the exercises create it in code.
    - Handler: create once, prepare when the host announces its block size (get_latency then gives the number for setLatencySamples), process in every callback. The places from step 8.
    - Click: the default stage takes a window out of the ring and writes the result back. A model that needs another shape, or its own pre- and post-processing, gets a stage in code; that is in Process.

---

<div class="tag">In the following</div>

## One map, three rows.<br><span class="then">A chapter per row, each from part 1 to anira.</span>

{{MAP}}

Note:
    - The map of the next chapters, empty: your app, the handler, the session behind it, the context with the pool. The rows are the three places in a plugin from step 8: the constructor, prepareToPlay, processBlock.
    - Each chapter fills one row. Every concept in it starts from the step in part 1 that built it by hand.
    - At the end the map is full, and the last chapter looks at it as one piece.
