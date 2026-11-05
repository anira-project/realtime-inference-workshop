<h1>Create</h1>

Plans, tensors, the pool

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- Create: plans, tensors and the pool.
- This is the first row of the map: what exists once the plugin is constructed.
- We go pair by pair: first what we built by hand in part 1, then what anira does instead.

---

<div class="tag">The map</div>

## Create runs once,<br><span class="then">on the main thread.</span>

{{MAP:focus=create}}

Note:
- Create runs once, on the main thread: the plugin's constructor.
- The files go in, the handler comes out.
- Behind the handler, the session will hold the plans and the state pairs, and the context the pool and the loaded models; this chapter builds each of them.
- Nothing in this row runs on the audio thread, so all of it may load files and allocate.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 1 and 5</div>

## Every engine has its own API.<br><span class="then">One class per engine, written by hand.</span>

<div class="tradeoff ships">
  <div class="tradeoff-name">LibTorch</div>
  <div class="tradeoff-bar weight" style="--v:100%;--i:0"><span>318 MB</span></div>
  <div class="tradeoff-name">ONNX Runtime</div>
  <div class="tradeoff-bar weight" style="--v:7.5%;--i:1"><span>24 MB</span></div>
  <div class="tradeoff-name">LiteRT</div>
  <div class="tradeoff-bar weight" style="--v:3%;--i:2"><span>~ 3 MB</span></div>
</div>

```cpp
class LibTorchEngine { … torch::jit::load … m_model.forward({input}) … };
class OnnxEngine     { … Ort::Session … m_session.Run(…) … };
```

Note:
- First the engines: in part 1 every engine has its own API, and we wrote one class per engine by hand.
- ONNX Runtime came in step 1, LibTorch next to it in the benchmark of step 4, and each handled the state differently.
- They also cost very different amounts to ship: LibTorch 318 MB, ONNX Runtime 24 MB, LiteRT about 3 MB.
- The plugin picked one of them at compile time.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Engines</div>

## One model file names every export.<br><span class="then">The handler keeps a plan per engine.</span>

```json
"models": [
  { "engine": "libtorch",    "path": "model.pt" },
  { "engine": "onnxruntime", "path": "model_forward.onnx" }
],
"default_engine": "onnxruntime"
```

<div class="statement-points">
  <div>LibTorch · ONNX Runtime · LiteRT · ExecuTorch: prebuilt, compiled in at build time</div>
  <div>Custom engines: your own code behind the same descriptor</div>
  <div>A plan is one entry on one engine and provider; <code>set_plan</code> switches between them</div>
</div>

Note:
- In anira one model file names every export, and the handler keeps a plan per engine.
- LibTorch, ONNX Runtime, LiteRT and ExecuTorch come prebuilt and are compiled in at build time.
- An entry for an engine the build does not have is skipped, so one file serves every build.
- A plan is one entry on one engine and execution provider, and set_plan switches between them.
- default_engine says which plan runs first.
- A custom engine is your own code behind the same interface, load, prepare and process, so a class like ours from part 1 fits there.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 1 and 3</div>

## Float buffers, one shape.<br><span class="then">The engine's tensor stays inside its class.</span>

```cpp
std::vector<float> output(input.begin(), input.end());   // step 1
m_model_block.assign(m_model_input_size, 0.0f);         // step 3
engine.process(m_model_block.data(), m_model_input_size);
```

Note:
- Next, tensors: in part 1 every buffer is a float vector with one shape the code assumes, one channel and one model block.
- That is step 1's output vector and step 3's model block.
- torch::Tensor and Ort::Value exist only inside the engine classes.
- A model with an int input, a second channel or an image does not fit through that call.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Tensors</div>

## One tensor type, on every slot.<br><span class="then">The model file says what each one is.</span>

```json
{ "name": "audio", "dtype": "float32", "role": "streamed",
  "axes": [["batch", 1], ["channel", 1], ["time", 2048]],
  "window": { "min": 2048, "max": 2048 } }
```

<div class="statement-points">
  <div><code>anira_tensor</code>: data, <code>dtype</code>, shape and strides, and where the memory lives</div>
  <div>Typed: float, int, bool and more, the DLPack codes; token IDs fit</div>
  <div>The spec in the model file: name, dtype, named axes, role</div>
</div>

Note:
- In anira there is one tensor type on every slot, and the model file says what each one is.
- anira_tensor is a plain C struct: a data pointer, a dtype, a shape and strides.
- The dtype codes are DLPack's, the standard that PyTorch and the other frameworks share, so int and bool fit, and so do token IDs.
- The spec in the model file names each slot, its dtype, its axes and its role.
- The axes have names, so anira knows which one is time and which one is channels, and adapts the host's blocks without guessing a layout.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 2</div>

## 63,488 floats of state,<br><span class="then">carried by hand.</span>

```cpp
// Carry the new state over to the next call. Hidden in LibTorch, ours here.
std::copy_n(outputs[1].GetTensorData<float>(), m_state.size(), m_state.begin());
```

Note:
- Then state: 63,488 floats of state, carried by hand.
- In the OnnxEngine from step 2 the state tensor comes out and we copy it into the state tensor going in, every call.
- In LibTorch the state is hidden, here it is ours.
- That is one copy of 63,488 floats per block, and the ordering of the calls is ours to guarantee.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · State</div>

## Every tensor has a role.<br><span class="then">State is one of them.</span>

```json
"inputs": [
  { "name": "audio",     "role": "streamed",
    "axes": [["batch",1],["channel",1],["time",2048]] },
  { "name": "state_in",  "role": "state",    "axes": [["batch",1],["any",63488]],
    "state_source": "state_out" }
], "outputs": [
  { "name": "audio_out", "role": "streamed", …, "latency": 2224 },
  { "name": "state_out", "role": "state",    "axes": [["batch",1],["any",63488]] }
]
```

<div class="statement-points">
  <div><code>streamed</code>: has a time axis, consumed window by window</div>
  <div><code>static</code>: one value per run, a gain, a conditioning vector</div>
  <div><code>state</code>: a pair; anira feeds the output back into the input, no copy</div>
</div>

Note:
- In anira every tensor has a role, and state is one of them.
- Streamed has a time axis and is consumed window by window.
- Static is one value per run, like a gain or a conditioning vector.
- State is a pair, declared once: state_out feeds state_in.
- anira binds two buffers to the pair and swaps them around every inference: what came out goes in next, with no copy and no code on your side.
- Because the next run needs this run's state, a stateful model never has two runs in flight: anira runs its jobs one after another, in order.
- The LibTorch export keeps its state inside the file, so its entry declares none, and the same model file carries both.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 6</div>

## A worker per instance.<br><span class="then">It grows with every instance the host adds.</span>

<div class="lanes">
  <div class="lane-label">audio thread</div>
  <div class="lane audio"></div>
  <div class="lane-label"></div>
  <div class="lane-gap">
    <div class="lane-queue to"><span>m_input</span><i></i></div>
    <div class="lane-queue from"><span>m_output</span><i></i></div>
  </div>
  <div class="lane-label">worker thread</div>
  <div class="lane worker"><div class="lane-run">model</div></div>
</div>

Note:
- Threads: in step 6 we had a worker per instance; the plugin mixes down to mono, so one is enough.
- One worker, two ring buffers, one engine: correct, but it does not scale.
- The worker belongs to the instance, and the host decides how many instances there are, so the threads grow with every instance it adds.

---

<!-- .slide: class="answer" -->

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
// once per process; the defaults: cores / 2, SPIN_BACKOFF
context.threads(4, ANIRA_WAIT_BLOCKING);
```

Note:
- In anira there is one pool per process, and every instance queues into it.
- The first context sizes the pool, and every later one shares it.
- Strictly, it is one pool per copy of anira: every instance of your plugin shares it, and another plugin that links its own anira has its own.
- Here, ten instances, one pool, four threads.
- All handlers share one lock-free queue: the audio thread enqueues a job and leaves, and a free thread takes the oldest.
- In the animation, an instance flashes when it enqueues, and a thread is lit while its model runs.
- The default is half the cores; the second argument is how the pool waits, which is the next slide.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 6</div>

## The worker polls, the audio thread never waits.<br><span class="then">One fixed way to wait, on both sides.</span>

```cpp
// the worker
if (m_input.available() < k_model.m_input_size) { sleep_for(100us); continue; }

// the audio thread
if (m_output.available() >= num_samples) { m_output.pop(samples, num_samples); }
else                                     { std::fill_n(samples, num_samples, 0.0f); }
```

Note:
- Waiting: the worker polls, and the audio thread never waits.
- These are two decisions we made in step 6 without naming them: how the worker waits for work, and that the audio thread never waits for the worker.
- If the output is not ready, the audio thread writes silence.
- The 100 µs sleep is a compromise: responsive enough, and it burns a little CPU while idle.

---

<!-- .slide: class="answer" -->

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
- In anira the pool waits for work, either spinning with backoff or blocking.
- SPIN_BACKOFF is the default: the audio thread only touches an atomic, and the idle pool pays by polling.
- BLOCKING parks idle threads on a semaphore: no CPU while idle.
- The price is on the audio thread: if a pool thread is asleep, the enqueue makes one wake syscall.
- Under load the threads rarely sleep, so that is rare; on an idle pool it is nearly every job.
- The choice is made once per process, in context.threads.
- [click] We recommend BLOCKING: one bounded wake instead of threads polling while idle.
- It takes no lock and never waits, but no OS documents the wake as real-time safe, RTSan does not check it, and we have not yet measured its worst case per platform.
- SPIN_BACKOFF stays the default because the OS clocks a sleeping core down, and whatever runs next on it starts slow; spinning keeps the core warm.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 8</div>

## An engine per instance.<br><span class="then">Every instance loads the model again.</span>

```cpp
if (m_engine == nullptr) {
    m_engine = std::make_unique<OnnxEngine>(WORKSHOP_ONNX_MODEL_PATH);
    …
}
```

Note:
- Memory: step 8's plugin mixes its input down to mono and runs one engine, but that engine belongs to the instance.
- Every instance loads the model again: ten instances hold ten copies of the same model.
- Memory grows with the instances the host adds, like the threads did.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Memory</div>

## Loaded models are pooled.<br><span class="then">The state stays with the instance.</span>

```json
"models": [ { "engine": "onnxruntime", "path": "model_forward.onnx" } ],
"max_instances": 4
```

<div class="statement-points">
  <div>One load per model and provider, shared by every handler that runs it</div>
  <div><code>max_instances</code> caps the engine instances; the default is one per pool thread</div>
  <div>Every enabled plan stays loaded: a file with two engines holds both</div>
</div>

Note:
- In anira loaded models are pooled, and the state stays with the instance.
- The model is loaded once per model and provider, and every handler that runs it shares that load.
- Each handler then runs it through its own engine instances, and max_instances caps how many.
- The default is one per pool thread, since more could never run at once.
- A model with a state pair, like ours, gets exactly one instance per handler, because its runs must stay in order.
- The declared state lives in the handler, not in the engine.
- The cost to know: prepare loads the model of every enabled plan, so a file with two engines holds both until the handler goes.

---

<div class="tag">The map</div>

## After create:<br><span class="then">plans, state pairs, a pool and the loaded models.</span>

{{MAP:show=create|memory}}

Note:
- After create we have plans, state pairs, a pool and the loaded models.
- Marked is what holds memory from here on: the state pairs in the session and the models the context pools.
- Nothing streams yet, because the handler does not know the host's blocks.
- That is the next row.

---

<div class="tag">Take away</div>

## Declare once.<br><span class="then">Share what can be shared.</span>

<div class="statement-points">
  <div>Engines are entries in a file; a plan per engine and provider</div>
  <div>Every slot is a typed tensor; state is a role</div>
  <div>One pool and one copy of each model, for every instance in the process</div>
</div>

Note:
- Declare once, and share what can be shared.
- Engines are entries in a file, with a plan per engine and provider.
- Every slot is a typed tensor, and state is a role.
- One pool and one copy of each model serve every instance in the process.
- Next: prepare, when the host says which blocks it will deliver.
