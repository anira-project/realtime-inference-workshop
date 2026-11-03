<h1>Create</h1>

Plans, tensors, the pool

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - The first row of the map: what exists once the plugin is constructed. Four pairs, each from the step in part 1 that built it by hand to what anira does instead.

---

<div class="tag">The map</div>

## Create runs once,<br><span class="then">on the main thread.</span>

{{MAP:focus=create}}

Note:
    - The row we fill now: the plugin's constructor. The files go in, the handler comes out. Behind it the session holds the plans and the state pairs, the context the pool and the loaded models.
    - Nothing in this row runs on the audio thread, so all of it may load files and allocate.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 4 and 7</div>

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
    - Two engines, two classes, and the state handled differently in each: LibTorch from step 1, ONNX Runtime from step 4. Step 7 weighed what they cost to ship; the plugin picked one at compile time.

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
    - Entries for engines the build does not have are skipped, so one file serves every build.
    - A plan is one model entry on one engine and provider. Prepare returns the plan report: which plans exist and which one runs, the first by default.
    - In alpha.2 one budget covers every plan. Per-plan warm-up and budgets follow with alpha.3; then the reported latency covers the slowest enabled plan.
    - Custom engine: a descriptor with load, prepare, process; the hand-written option from step 7 fits here.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 1 and 2</div>

## Float buffers, one shape.<br><span class="then">The engine's tensor stays inside its class.</span>

```cpp
std::vector<float> output(input.begin(), input.end());   // step 1
m_model_block.assign(m_model_input_size, 0.0f);         // step 2
engine.process(m_model_block.data(), m_model_input_size);
```

Note:
    - Every buffer in part 1 is a float vector with one shape the code assumes: one channel, one model block. torch::Tensor and Ort::Value exist only inside the engine classes.
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
    - One passive struct for every input and output, the same 216 bytes on every platform. It carries the memory, its owner and whether it is ready; anira never guesses a layout.
    - The domain says where the memory lives. In alpha.2 that is host memory; the GPU domains follow with M4.
    - The spec is the model's truth per slot: the named axes tell anira which one is time and which one is channels, so it can adapt the host's blocks to the window.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · ONNX</div>

## 63,488 floats of state,<br><span class="then">carried by hand.</span>

```cpp
// Carry the new state over to the next call. Hidden in LibTorch, ours here.
std::copy_n(outputs[1].GetTensorData<float>(), m_state.size(), m_state.begin());
```

Note:
    - OnnxEngine from step 4: the state tensor out, copied into the state tensor in, every call. 63,488 floats, one copy per block, and the ordering of calls is ours to guarantee.

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
    - The state pair is declared once. anira binds two buffers to it and swaps them around every inference: what came out is what goes in next. No copy, no code on your side.
    - A model with a state pair is stateful whatever else it says: one job in flight, in order.
    - The LibTorch export keeps its state inside the file; its model entry says nothing about it, and the same model file carries both.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 5</div>

## A worker per channel, per instance.<br><span class="then">It grows with every instance the host adds.</span>

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
    - Step 5's picture. One worker, two queues, one engine. Correct, and it does not scale: the worker belongs to the instance, and the host decides how many instances there are.

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
context.threads(4, ANIRA_WAIT_SPIN_BACKOFF);   // once per process; the default: cores / 2
```

Note:
    - The pool belongs to anira's core, one per process: the first context sizes it, every later one shares it. Ten plugins, one pool, four real-time threads.
    - One lock-free queue (moodycamel) for every handler. The audio thread enqueues a job and leaves; a free thread takes the oldest one. The animation shows it: instances flash when they enqueue, the threads take turns, each busy while its model runs.
    - A stateful model never has two jobs in flight: its next job is enqueued by the thread that finished the last one, so the order holds without a lock.
    - Thread count: hardware concurrency / 2 by default. Zero means the host brings its own threads.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 5</div>

## The worker polls, the audio thread never waits.<br><span class="then">One fixed way to wait, on both sides.</span>

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
    - An idle pool thread has to wait for the next job somehow. SPIN_BACKOFF (default) tries a few times, then pauses the core, then yields, then sleeps 100 µs: exponential backoff, Timur Doumler's recipe from ADC20. Fast to react, costs a little CPU while idle.
    - BLOCKING parks the thread on a semaphore and the enqueue wakes it. No CPU while idle, a little more time to wake up.
    - The choice is the context's, once per process: context.threads(4, ANIRA_WAIT_SPIN_BACKOFF).
    - The cost lands on different threads. With SPIN_BACKOFF the audio thread only does an atomic enqueue; the idle pool pays by polling. With BLOCKING the idle pool costs nothing, but when a pool thread is asleep the enqueue on the audio thread wakes it: sem_post (futex) on Linux and Android, semaphore_signal (a Mach trap) on Apple, ReleaseSemaphore on Windows. Under load the threads rarely sleep and the wake is rare; on an idle pool it happens for nearly every job.
    - Click: our recommendation is BLOCKING. The wake is bounded and never waits, unlike a lock. But no platform documents it as real-time safe, and RTSan intercepts pthread_mutex_unlock and pthread_cond_signal, not sem_post or semaphore_signal, so it will not tell you. We have not measured its worst case per platform yet.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 8</div>

## An engine per channel, per instance.<br><span class="then">Every one loads the model again.</span>

```cpp
while (m_engines.size() < channels) {
    m_engines.push_back(std::make_unique<OnnxEngine>(WORKSHOP_ONNX_MODEL_PATH));
    …
}
```

Note:
    - Step 8's plugin loads one ONNX session per channel, because the state belongs to the signal running through it. Ten stereo instances hold twenty copies of the same model.
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
    - The declared state pairs live in each handler's session, not in the engine, so one loaded model can serve every instance. Ten stereo plugins, one model file: as many engine instances as there are pool threads, not twenty.
    - max_instances is a ceiling on memory: the planner fits the engine instances under it. More instances than threads would never run at once.
    - The cost to know: prepare loads the model of every enabled plan, and all of them stay resident until the handler goes.

---

<div class="tag">The map</div>

## After create:<br><span class="then">plans, state pairs, a pool and the loaded models.</span>

{{MAP:show=create|memory}}

Note:
    - The create row, filled. Marked: what holds memory from here on, the state pairs in the session and the models the context pools.
    - Nothing streams yet: the handler does not know the host's blocks. That is the next row.

---

<div class="tag">Take away</div>

## Declare once.<br><span class="then">Share what can be shared.</span>

<div class="statement-points">
  <div>Engines are entries in a file; a plan per engine and provider</div>
  <div>Every slot is a typed tensor; state is a role</div>
  <div>One pool and one copy of each model, for every instance in the process</div>
</div>

Note:
    - Next: prepare, when the host says which blocks it will deliver.
