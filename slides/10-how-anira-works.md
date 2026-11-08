<h1>How anira works</h1>

Create, prepare, process

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- Thanks valentin for the history overview of anira
- In the next part I want to show you the inner workings of the anira library
- You start using anira, by declaring up to three configs
- And then the InferenceHandler is your API to anira's inference thread pool.

---

<div class="tag">How you use it</div>

## Three configs: the model, the stream, the pool.<br><span class="then">The handler runs it.</span>

<div class="cards">
  <div class="card">{{ICON:model}}<div class="card-title">ModelConfig</div><div class="card-text">the model file or binary, every tensor and its role</div><div class="card-file">model.json</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:sliders}}<div class="card-title">Contract</div><div class="card-text">the host's block sizes and sample rate, the inference budget</div><div class="card-file">contract.json</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:chip}}<div class="card-title">ContextConfig</div><div class="card-text">the thread pool, its threads, the log level</div><div class="card-file optional">context.json · optional</div></div>
  <div class="card-arrow">→</div>
  <div class="card accent">{{ICON:shield}}<div class="card-title">Handler</div><div class="card-text">your API to the inference pool</div><div class="card-file">create · prepare · process</div></div>
</div>

<div class="cards-note">A model file and a contract file, plus an optional context file, and three calls. For most models that is all.</div>

Note:
- There is the ModelConfig (remember the constant struct at the beginning of the examples), a stream contract, a ContextConfig.
- The ModelConfig declares the model: its file path or binary, and every tensor with its role.
- The Contract is the contract for the stream: it contains information about the host's buffer sizes and sample rate, and the inference budget.
- The optional ContextConfig declares the context of the process: how many threads the pool has and which log level.
- The InferenceHandler then gives you the three typical calls from step 8: create, prepare and process. It is created with the model config and the context; the contract comes in at prepare, once the host has said its buffer size.

---

<div class="tag">In the following</div>

## The three calls in a plugin,<br><span class="then">now with anira behind them.</span>

{{MAP}}

Note:
- The three handler calls, create, prepare and process, can be directly mapped onto the plugin calls from step 8: the constructor, prepareToPlay and processBlock.
- The columns run from your code on the left, through the language api of your choice (C++, Typescript, C), through the C ABI, to anira's private session and context on the right
- Typically create and prepare run on the main thread and the process function is called from the audio thread.
- In the following we will fill each row
- We'll start with the implementations from the first workshop part and see how its implemented in anira for general use case
