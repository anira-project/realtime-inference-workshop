<h1>How anira works</h1>

Three configs, then the design

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- From what anira is to how it works.
- First the three configs you declare, then the design, over the next three chapters.

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

<div class="cards-note">A model file and a contract file, plus an optional context file, and three calls. For most models that is all.</div>

<div class="cards-note fragment">Some models need their own pre- and post-processing: a stage, in code. That comes later.</div>

Note:
- Three configs say what, how and with what, and the handler runs it.
- ModelConfig is the what: the model files and every tensor, each with a name and a role.
- The contract is the how: what the host promises, and what it gets back.
- Hard is the contract for a stream, and it is the one we use all day; Async, for jobs, comes later.
- ContextConfig is the with what: threads, engines and the log, one per process, and the defaults fit most hosts.
- So that is a model file and a contract file, plus an optional context file.
- The handler takes three calls, in the three places from step 8: create in the constructor, prepare in prepareToPlay, process in processBlock.
- For most models, that is all you write.
- [click] A model that needs its own pre- and post-processing gets a stage in code, and that comes in the process chapter.

---

<div class="tag">In the following</div>

## One map, three rows.<br><span class="then">A chapter per row, each from part 1 to anira.</span>

{{MAP}}

Note:
- One map, three rows, and a chapter per row.
- The map is still empty: your app, the handler, the session behind it, and the context with the pool.
- The columns run from your code on the left, through the handler, which is the public C ABI, to anira's private session and context on the right; we come back to the ABI at the end.
- The top band is the main and the audio thread, the bottom one the inference threads.
- The rows are the three places in a plugin from step 8: the constructor, prepareToPlay and processBlock.
- Each chapter fills one row, and every concept in it starts from the step in part 1 where we built it by hand.
- At the end the map is full, and the last chapter looks at it as one piece.
