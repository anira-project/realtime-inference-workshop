<h1>Create</h1>

What you declare: engines, tensors, state, the pool

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- Create: what we declare, the engines, the tensors, the state and the pool.
- This is the first row of the map: what exists once the plugin is constructed.
- We go pair by pair: first what we built by hand in part 1, then what anira does instead.

---

<div class="tag">The map</div>

## Create runs once,<br><span class="then">on the main thread.</span>

{{MAP:focus=create}}

Note:
- Create runs once, on the main thread: the plugin's constructor.
- The files go in, the handler comes out.
- Create checks the files and builds the handler's slots, one per tensor, the state pairs among them.
- Nothing is loaded yet: the models, the plans and the pool's threads come at prepare. This chapter is about what we declare for them.
- Nothing in this row runs on the audio thread, so all of it may load files and allocate.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 1 and 5</div>

## One engine, one class.<br><span class="then">Written by hand.</span>

```cpp
class OnnxEngine { … Ort::Session … m_session.Run(…) … };   // step 1
```

Note:
- First the engines: in part 1 we wrote one class for one engine, ONNX Runtime, by hand.
- It loads the session, owns the tensors, copies the state, and runs the model.
- Another engine, LibTorch, ExecuTorch, LiteRT or your own code, would mean another class like it, with its own API and its own way of handling the state.
- The plugin is built for that one engine, at compile time.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Engines</div>

## anira brings the engines.<br><span class="then">Prebuilt, static or shared. Or bring your own.</span>

<div class="tradeoff ships">
  <div class="tradeoff-name">LibTorch</div>
  <div class="tradeoff-bar weight" style="--v:100%;--i:0"><span>318 MB</span></div>
  <div class="tradeoff-name">ONNX Runtime</div>
  <div class="tradeoff-bar weight" style="--v:8.2%;--i:1"><span>26 MB</span></div>
  <div class="tradeoff-name">ExecuTorch</div>
  <div class="tradeoff-bar weight" style="--v:5.3%;--i:2"><span>17 MB, static only</span></div>
  <div class="tradeoff-name">LiteRT</div>
  <div class="tradeoff-bar weight" style="--v:3.5%;--i:3"><span>11 MB</span></div>
  <div class="tradeoff-name">TensorFlow Lite</div>
  <div class="tradeoff-bar weight" style="--v:1.6%;--i:4"><span>5 MB</span></div>
  <div class="tradeoff-name">Custom</div>
  <div class="tradeoff-bar weight" style="--v:0.6%;--i:5"><span>depends on your engine</span></div>
</div>

<div class="cards-note">Behind anira's C ABI with hidden symbols: no clash with a host or another plugin that ships its own engine.</div>

Note:
- anira brings the engines: LibTorch, ONNX Runtime, ExecuTorch, LiteRT and TensorFlow Lite, prebuilt, static or shared, compiled in at build time.
- Each one is an adapter behind the same interface: what we wrote by hand as OnnxEngine, once per engine.
- Or bring your own: a custom engine is your own code behind the same descriptor, load, prepare and process, so a class like ours from part 1 fits there. Its size is whatever your engine weighs.
- They cost very different amounts to ship: LibTorch 318 MB, ONNX Runtime 26 MB, ExecuTorch 17 MB, LiteRT 11 MB, TensorFlow Lite 5 MB.
- Measured: the libraries unpacked from the anira-project/backends releases, macOS arm64 (LibTorch 2.12.0, ONNX Runtime 1.30.0, ExecuTorch 1.3.1, LiteRT 2.2.0, TensorFlow Lite 2.17.0). LibTorch ships only shared, ExecuTorch only static; less of a static library ends up in a linked plugin.
- anira's public interface is a C ABI, and everything else is hidden: only the anira_ symbols are exported. So two plugins with different versions of anira, next to a host that ships its own ONNX Runtime, do not clash.
- An entry for an engine the build does not have is skipped.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Engines</div>

## One model config for every engine.<br><span class="then">Each entry loads its own export.</span>

```json
"models": [
  { "engine": "onnxruntime", "path": "model_forward.onnx" },
  { "engine": "libtorch",    "path": "model.pt" },
  { "engine": "executorch",  "path": "model.pte" },
  { "engine": "litert",      "path": "model.tflite" }
],
"default_engine": "onnxruntime"
```

<div class="statement-points">
  <div>One entry per export; the default engine is the one that runs</div>
  <div>Every entry takes the same tensors, the state included</div>
  <div>An entry the build cannot run is skipped: one file for every build</div>
</div>

Note:
- How to configure it: you list them in one model config, one entry per engine, each with its own export of the model.
- Here four: ONNX Runtime, LibTorch, ExecuTorch and LiteRT, each loading its own file.
- default_engine says which one runs. You can benchmark the model on each engine, from the same config.
- Every entry has to take the same tensors, state included.
- An entry for an engine the build does not have is skipped, so one file serves every build.
- The plans are built at prepare, when the models load. Create checks the model config's structure: the tensors, the axes, and which entries this build can run.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Engines</div>

## For our example, ONNX Runtime only.<br><span class="then">To keep it simple.</span>

```json
"models": [
  { "engine": "onnxruntime", "path": "path/to/model_forward.onnx" }
]
```

<div class="cards-note">Step 9's <code>model.json</code>: one entry, one export. A relative path is resolved against the folder of the config file.</div>

Note:
- For our example we use ONNX Runtime only, to keep it simple: one entry, one export.
- The workshop builds only ONNX Runtime, so another entry would be skipped anyway.
- And our model.pt keeps its state inside: next to this entry it would need an export with the state in and out.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 1 and 3</div>

## Float buffers, one shape.<br><span class="then">The engine's tensor stays inside its class.</span>

```cpp
std::vector<float> audio(k_input_signal.begin(), k_input_signal.end());   // step 1
m_model_block.assign(k_model.m_input_size, 0.0f);                        // step 3
m_state.assign(k_model.m_state_size, 0.0f);                              // the state, too
m_engine.process(m_model_block.data(), k_model.m_input_size, m_state);
```

Note:
- Next, tensors: in part 1 every buffer is a float vector with one shape the code assumes, one channel and one model block.
- That is step 1's audio vector, step 3's model block, and the state: 63,488 floats in one more vector.
- Ort::Value exists only inside OnnxEngine.
- A model with an int input, a second channel or an image does not fit through that call.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Tensors</div>

## One tensor type, on every slot.<br><span class="then">The model file says what each one is.</span>

```json
{ "name": "audio", "dtype": "float32", "role": "streamed",
  "axes": [["batch", 1], ["channel", 1], ["time", 2048]] }
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
// OnnxEngine::process: carry the new state over to the next call, every block.
std::copy_n(m_state_out.begin(), m_state_out.size(), state.begin());
```

Note:
- Then state: 63,488 floats of state, carried by hand.
- In the OnnxEngine from step 2 the state tensor comes out and we copy it into the state tensor going in, every call.
- A TorchScript export can hide the state inside the model; an ONNX export cannot, so here it is ours.
- That is one copy of 63,488 floats per block, and the ordering of the calls is ours to guarantee.

---

<!-- .slide: class="answer dense-code" -->

<div class="tag">anira · State</div>

## Every tensor has a role.<br><span class="then">State is one of them.</span>

```json
"inputs": [
  { "name": "audio",     "role": "streamed",
    "axes": [["batch",1],["channel",1],["time",2048]] },
  { "name": "state_in",  "role": "state",
    "axes": [["batch",1],["any",63488]], "state_source": "state_out" }
], "outputs": [
  { "name": "audio_out", "role": "streamed",
    "axes": [["batch",1],["channel",1],["time",2048]], "latency": 2224 },
  { "name": "state_out", "role": "state", "axes": [["batch",1],["any",63488]] }
]
```

<div class="statement-points">
  <div><code>streamed</code>: has a time axis, consumed window by window</div>
  <div><code>static</code>: one value per run, a gain, a conditioning vector</div>
  <div><code>state</code>: a pair; anira feeds the output back into the input, no copy</div>
</div>

Note:
- In anira every tensor has a role, and state is one of them.
- This is the whole declaration of our model, nothing left out. The dtype is not written: float32 is the default.
- Streamed has a time axis and is consumed window by window.
- Static is one value per run, like a gain or a conditioning vector.
- State is a pair, declared once: state_out feeds state_in.
- anira binds two buffers to the pair and swaps them around every inference: what came out goes in next, with no copy and no code on your side.
- Because the next run needs this run's state, a stateful model never has two runs in flight: anira runs its jobs one after another, in order.
- The tensors are declared once, for every entry of the file: each engine's export has to take the state in and give it out, like our ONNX file.

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
anira::ContextConfig config;
config.threads(4, ANIRA_WAIT_BLOCKING);
anira::Context context{config};
```

Note:
- In anira there is one pool per process, and every instance queues into it.
- The first one sizes the pool, and every later one shares it; one asking for fewer threads shrinks it. The threads run while a handler is prepared.
- Strictly, it is one pool per copy of anira: every instance of your plugin shares it, and another plugin that links its own anira has its own.
- Here, ten instances, one pool, four threads.
- All handlers share one lock-free queue: the audio thread enqueues a job and leaves, and a free thread takes the next one. The order holds within each handler.
- In the animation, an instance flashes when it enqueues, and a thread is lit while its model runs.
- The default is half the cores; the second argument is how the pool waits, which is the next slide.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 6</div>

## The worker spins, the audio thread never waits.<br><span class="then">One fixed way to wait, on both sides.</span>

```cpp
// the worker hot-spins: no block yet, it checks again at once, no sleep
if (m_input.available() >= k_model.m_input_size) { /* pop, run the model, push */ }

// the audio thread: one sample at a time, never waits
const bool popped = m_debt == 0 && m_output.try_pop(samples[i]);
if (!popped) { /* zeros for the rest of the callback */ m_debt += num_samples - i; }
```

Note:
- Waiting: the worker spins, and the audio thread never waits.
- These are two decisions we made in step 6 without naming them: how the worker waits for work, and that the audio thread never waits for the worker.
- The worker hot-spins: no sleep, no pause, one core at 100 percent, even while idle.
- If a sample is not there yet, the audio thread writes zeros for the rest of the callback and owes them: the debt.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Waiting</div>

## The pool waits for work.<br><span class="then">Spin with backoff, or block.</span>

<div class="policies">
  <div class="policy-head"></div><div class="policy-head"></div><div class="policy-head">audio thread, on enqueue</div><div class="policy-head">pool, while idle</div>

  <div class="policy-name"><code>SPIN_BACKOFF</code></div>
  <div class="wait-bar spin"><i></i><i></i><i></i><i></i><i></i><i></i><i></i><i></i></div>
  <div class="policy-cell ok">an atomic, nothing else</div>
  <div class="policy-cell warn">polls: 4 retries, 32 with a CPU pause, then yield and sleep 100 µs</div>

  <div class="policy-name"><code>BLOCKING</code></div>
  <div class="wait-bar block"><i></i></div>
  <div class="policy-cell warn">an atomic, plus one wake syscall when a thread is blocked</div>
  <div class="policy-cell ok">spins up to 10,000 tries, then blocks on a semaphore</div>
</div>

<div class="policy-advice fragment"><span>We recommend BLOCKING.</span> One bounded wake instead of threads polling while idle. It takes no lock and never waits, but no OS documents it as real-time safe, and RTSan does not check it.</div>

Note:
- In anira the pool waits for work, either spinning with backoff or blocking.
- SPIN_BACKOFF is the default: the audio thread only touches an atomic, and the idle pool pays by polling.
- Its backoff: 4 retries at once, 32 more with a CPU pause hint of about 100 ns each, then a yield and a 100 µs sleep before every further retry. So an idle thread picks up new work within about 100 µs.
- BLOCKING waits in moodycamel's semaphore: it first spins on the semaphore's count, up to 10,000 tries without a pause, then blocks on the OS semaphore until an enqueue wakes it.
- It wakes up every 5 ms on its own, only to check whether it should exit. While blocked, it uses no CPU.
- The price is on the audio thread: if a pool thread is blocked, the enqueue makes one wake syscall.
- Under load the threads rarely sleep, so that is rare; on an idle pool it is nearly every job.
- The choice is made once per process, in ContextConfig::threads.
- [click] We recommend BLOCKING: one bounded wake instead of threads polling while idle.
- It takes no lock and never waits, but no OS documents the wake as real-time safe, RTSan does not check it, and we have not yet measured its worst case per platform.
- SPIN_BACKOFF stays the default because the OS clocks a sleeping core down, and whatever runs next on it starts slow; spinning keeps the core warm.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 8</div>

## An engine per instance.<br><span class="then">Every instance loads the model again.</span>

```cpp
if (m_engine == nullptr) {
    m_engine = std::make_unique<OnnxEngine>(k_model.m_path);
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

## Our model: one instance per handler.<br><span class="then">Stateless models can run in parallel.</span>

<div class="cards">
  <div class="card accent">{{ICON:model}}<div class="card-title">With state, like ours</div><div class="card-text">One instance per handler, whatever the config says. Its jobs run one at a time, in order; anira carries the state.</div></div>
  <div class="card">{{ICON:threads}}<div class="card-title">Without state</div><div class="card-text">Loaded once for every handler. <code>"max_instances": 4</code> runs up to four inferences at once on the pool.</div></div>
</div>

Note:
- What anira does for a model like ours, with state: one engine instance per handler, whatever max_instances says, and its jobs run one after another, in order, because each needs the state of the one before. That is anira 3's rule for every stateful model or declared state pair.
- Compared to part 1: anira carries the state, keeps the order, and the threads come from the shared pool instead of one worker per instance.
- The instance itself is not shared: with ONNX Runtime it is its own session with its own copy of the weights, so ten plugin instances still hold ten copies, as in step 8.
- For a model without state anira can do more: it is loaded once and shared by every handler, and max_instances lets several inferences of it run at the same time on the pool. That multithreads stateless inference, which helps when a block has more work than one thread can do in time.
- The default for max_instances is one per pool thread, since more could never run at once.
- Shared means the whole load matches: engine, path, provider, options and instance count.
- Prepare loads the model of every enabled plan, so a file with two engines holds both until the handler is destroyed or prepared again.

---

<div class="tag">The map</div>

## After create:<br><span class="then">checked files, slots and state pairs.</span>

{{MAP:show=create|memory}}

Note:
- After create we have checked files, a slot per tensor, and the state pairs.
- Marked is what holds memory from here on: the state pairs, both buffers of each.
- Nothing is loaded yet: no model, no plan, no thread. All of that needs the contract, and the contract comes with the host's blocks.
- That is the next row.

---

<div class="tag">Take away</div>

## Declare once.<br><span class="then">Share what can be shared.</span>

<div class="statement-points">
  <div>Engines are entries in a file; a plan per engine and provider</div>
  <div>Every slot is a typed tensor; state is a role</div>
  <div>One pool for every instance in the process; models without state are shared</div>
</div>

Note:
- Declare once, and share what can be shared.
- Engines are entries in a file, with a plan per engine and provider.
- Every slot is a typed tensor, and state is a role.
- One pool serves every instance in the process, and a model without state is loaded once for all of them.
- Next: prepare, when the host says which blocks it will deliver.
