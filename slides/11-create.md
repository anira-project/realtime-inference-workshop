<h1>Create</h1>

What you declare: the model and the pool. Nothing runs yet.

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- The first row is about the plugin's constructor where we create the InferenceHandler
- One rule for the whole chapter: create declares and checks. Nothing we declare here takes effect before prepare or process

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 1 and 8</div>

## One engine, one class.<br><span class="then">Chosen at compile time.</span>

```cpp
class OnnxEngine {                              // step 1: ONNX Runtime, and only that
    Ort::Env m_env{nullptr};
    Ort::Session m_session{nullptr};
    std::vector<Ort::Value> m_inputs, m_outputs;   // audio, state
    …
};

class ProcessorExample {
    OnnxEngine m_engine;                        // step 8: a member, loaded in prepare()
    …
};
```

Note:
- Lets recap the constructors in the first workshop phase
- First the engines.
- In part 1 we wrote one class for one engine, ONNX Runtime, by hand: it owns the session and the tensors, and later loads and runs the model.
- The engine was a decision of the code: another engine, LibTorch, ExecuTorch, LiteRT or your own, would be another class like it, with its own API and its own way of handling the state. The plugin is built for one engine, at compile time.
- Starting from step 3 the ProcessorExample is owning the class
- And in step 8 it was constructed with the plugin's constructor

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Engines</div>
<div class="when">→ <b>prepare</b>: the model loads</div>

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
- anira on the other hand comes with multiple engines out of the box: LibTorch, ONNX Runtime, ExecuTorch, LiteRT and TensorFlow Lite, prebuilt .. as static or shared and compiled in or linked at build time.
- Each one is an adapter behind a unified interface that has four calls, descriptor, load, prepare and process
- You can also implement your own engine through the interface, so a class like ours from part 1 fits there as it had similar calls
- One thing worth mentioning: anira keeps the engines' symbols private. They are hidden when your plugin is linked, and the plugin exports only anira's C API.
- That matters because some hosts ship their own engine; Ableton Live 12, for example, bundles its own ONNX Runtime. If your plugin exported its engine symbols, the dynamic linker could bind your calls to the host's copy, a different version, and the host can crash on the first call.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Engines</div>
<div class="when">→ <b>prepare</b>: the default entry loads</div>

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
- So if those engines come out of the box, how do you declare which one your model needs?
- In anira you declare this in the model config
- Here is an example: we have four different model exports and all of them need different engines.
- Then in the end we can define a default engine which will be the engine choice after prepare
- At runtime you can then switch between the engines
- If you define an entry for an engine the build does not it is skipped, so one file serves every build.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Engines</div>
<div class="when">→ <b>prepare</b>: this entry loads</div>

## For our example, ONNX Runtime only.<br><span class="then">To keep it simple.</span>

```json
"models": [
  { "engine": "onnxruntime", "path": "path/to/model_forward.onnx" }
]
```

<div class="cards-note">Step 9's <code>model.json</code>: one entry, one export. A relative path is resolved against the folder of the config file.</div>

Note:
- I.e. model we have used in part 1 would be declared like this
- Just a sidenote, anira also supports binary model files compiled in

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 1, 2 and 7</div>

## A model config, by hand.<br><span class="then">Five facts. The code assumes the rest.</span>

```cpp
constexpr struct ModelConfig {             // from the export metadata, step 1
    const char* m_path = WORKSHOP_ONNX_MODEL_PATH;
    size_t m_input_size = 2048;            // the model's block
    size_t m_state_size = 63488;           // step 2
    double m_sample_rate = 44100.0;
    size_t m_latency = 2224;               // step 7
} k_model{};

ProcessorExample m_processor{k_model};     // step 8: given at construction, kept
```

Note:
- But if we recap our the constant k_model struct there was more information than just the model path.
- The more and more complex our examples grew, the more members our struct got:
- First only input size, then also state, latency, etc...
- Then there were also information that our Onnx Engine was assuming, i.e. the data type (float in our example, that we only had one channel, etc.)
- Anira generalizes this the Tensor specifications in the following way, the tensor specifications are part of the model config as they are defined by the model

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Tensors</div>
<div class="when">→ <b>process</b>: the blocks go through the slots</div>

## One tensor type, on every slot.<br><span class="then">The model file says what each one is.</span>

```json
{ "name": "audio_in", "dtype": "float32", "role": "streamed",
  "axes": [["batch", 1], ["channel", 1], ["time", 2048]] }
```

<div class="statement-points">
  <div><code>anira_tensor</code>: data, <code>dtype</code>, shape and strides, and where the memory lives</div>
  <div>Typed: float, int, bool and more, the DLPack codes; token IDs fit</div>
  <div>The spec in the model file: name, dtype, named axes, role</div>
</div>

Note:
- In anira every tensor - in or out - has a slot with a name and role
- I.e. our input audio is a streamed tensor (it needs to be feed as a continuous stream with a time axis)


- anira_tensor is a plain C struct: a data pointer, a dtype, a shape and strides.
- The dtype codes are DLPack's, the standard that PyTorch and the other frameworks share, so int and bool fit, and so do token IDs.
- The spec in the model file names each slot, its dtype, its axes and its role.
- The axes have names, so anira knows which one is time and which one is channels, and adapts the host's blocks without guessing a layout.
- Create checks the shapes and builds a slot per tensor. The host's blocks go through them in process.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 2</div>

## 63,488 floats of state,<br><span class="then">a size and two tensors. Who feeds whom is in the code.</span>

```cpp
size_t m_state_size = 63488;                      // ModelConfig, step 2

std::vector<Ort::Value> m_inputs;   // audio, state       OnnxEngine
std::vector<Ort::Value> m_outputs;  // audio, state
// the state output feeds the state input: a copy in process(), by hand
```

Note:
- Then the state. In part 1 it was declared as a size and two tensors in the engine, and that was all the code could say about it.
- That the state output feeds the state input was declared nowhere: OnnxEngine::process copies it over, every block, by hand, 63,488 floats, and the order of the calls is ours to guarantee.
- A TorchScript export can hide the state inside the model; an ONNX export cannot, so here it is ours.

---

<!-- .slide: class="answer dense-code" -->

<div class="tag">anira · State</div>
<div class="when">→ <b>process</b>: swapped after every run</div>

## Every tensor has a role.<br><span class="then">State is one of them.</span>

```json
"inputs": [
  { "name": "audio_in",  "role": "streamed",
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
- State is a pair, declared once: state_out feeds state_in. Create resolves the pair and gives it its two buffers.
- What happens with them is process's: after every run anira swaps them, what came out goes in next, no copy and no code of yours. The stages slide shows it.
- The model's own latency is declared here too, on the output: 2224 samples. Prepare adds it to the latency the host is told.
- The tensors are declared once, for every entry of the file: each engine's export has to take the state in and give it out, like our ONNX file.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 6</div>

## A worker per instance.<br><span class="then">Fixed by the class. The host decides how many instances.</span>

```cpp
class ProcessorExample {                     // step 6
    …
    ThreadSafeRingBuffer m_input{0}, m_output{0};
    WorkerThread m_worker;                   // one thread per processor
};

ProcessorExample m_processor{k_model};       // step 8: one processor per plugin instance
```

Note:
- Threads. In step 6 the worker is a member of the processor: one thread per processor, and in step 8 one processor per plugin instance.
- One worker, two ring buffers, one engine: correct, but it does not scale. The host decides how many instances there are, so the threads grow with every instance it adds.
- How that worker waits for work, and that the audio thread never waits for it, were two more decisions of the code. We look at them in Process.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 6</div>

## The worker spins, the audio thread never waits.<br><span class="then">One fixed way to wait, on both sides.</span>

{{THREADS}}

```cpp
// the worker hot-spins: a sample not there yet, it tries again at once, no sleep
while (!m_input.try_pop(m_model_block[i])) { if (!m_worker.running()) { return; } }
// the audio thread never waits: a sample not there yet is a zero, and a debt
const bool popped = m_debt == 0 && m_output.try_pop(samples[i]);
```

Note:
- In step 6 we made two decisions without naming them: how the worker waits for work, and that the audio thread never waits for the worker.
- The timeline from step 6: between two runs the worker spins on try_pop the whole time, no sleep, no pause, one core at 100 percent, even while idle.
- The audio thread only pushes and pops. If a sample is not there yet, it writes zeros for the rest of the callback and owes them: the debt.
- Both were fixed in the code, the same for every instance.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Threads</div>
<div class="when">→ <b>prepare</b>: the threads start</div>

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

```json
// context.json: once per process; the defaults: cores / 2, "spin_backoff"
{ "num_threads": 4, "wait_strategy": "blocking" }
```

Note:
- In anira there is one pool per process, and every instance queues into it.
- Declared once, in the ContextConfig: how many threads, and how they wait for work while idle.
- The first one sizes the pool, and every later one shares it; one asking for fewer threads shrinks it. The threads start with the first prepare and run until the last handler is destroyed.
- Strictly, it is one pool per copy of anira: every instance of your plugin shares it, and another plugin that links its own anira has its own.
- Here, ten instances, one pool, four threads. In the animation, an instance flashes when it enqueues, and a thread is lit while its model runs.
- How a job travels from the audio thread through the queue to a thread: Process.
- The default is half the cores. The second argument, how the idle threads wait, we come back to in Process as well, next to the audio thread's own wait.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Pool</div>
<div class="when">→ <b>process</b>: the threads wait for jobs</div>

## The pool waits for work.<br><span class="then">Spin with backoff, or block. Declared once, at create.</span>

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
- In anira the pool's wait is one setting for the whole process, declared at create in the ContextConfig, its wait_strategy: spin with backoff, or block.
- Not to be confused with wait_ratio, which comes in Process: the audio thread waiting for a result, per handler, in the contract. This is the pool waiting for work.
- SPIN_BACKOFF is the default: the audio thread only touches an atomic, and the idle pool pays by polling.
- Its backoff: 4 retries at once, 32 more with a CPU pause hint of about 100 ns each, then a yield and a 100 µs sleep before every further retry. So an idle thread picks up new work within about 100 µs.
- BLOCKING waits in moodycamel's semaphore: it first spins on the semaphore's count, up to 10,000 tries without a pause, then blocks on the OS semaphore until an enqueue wakes it.
- It wakes up every 5 ms on its own, only to check whether it should exit. While blocked, it uses no CPU.
- The price is on the audio thread: if a pool thread is blocked, the enqueue makes one wake syscall.
- Under load the threads rarely sleep, so that is rare; on an idle pool it is nearly every job.
- [click] We recommend BLOCKING: one bounded wake instead of threads polling while idle.
- It takes no lock and never waits, but no OS documents the wake as real-time safe, RTSan does not check it, and we have not yet measured its worst case per platform.
- SPIN_BACKOFF stays the default because the OS clocks a sleeping core down, and whatever runs next on it starts slow; spinning keeps the core warm.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 8</div>

## An engine per instance.<br><span class="then">Ten instances, ten copies of the model.</span>

```cpp
class PluginProcessor final : public PluginBase {
    …
    ProcessorExample m_processor{k_model};  // one per instance, its engine inside
};

size_t PluginProcessor::prepare(…) {
    // every instance loads the model: ten instances, ten copies
    m_processor.prepare(host_block_size, k_max_inference_ms);
    …
}
```

Note:
- Memory. In step 8 the processor is a member of the plugin, and the engine a member of the processor: one engine per instance, fixed by the classes.
- Each instance loads the model in its prepare, so ten instances hold ten copies of the same model. Memory grows with the instances the host adds, like the threads did.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Memory</div>
<div class="when">→ <b>prepare</b>: the instances load</div>

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
- All of it is declared here and loaded at prepare: a file with two enabled engines holds both. Prepare's memory slide.

---

<div class="tag">After create</div>

## Declared and checked.<br><span class="then">In effect from prepare on.</span>

<table class="declared">
  <thead>
    <tr><th>You declare</th><th>Create</th><th>Takes effect</th></tr>
  </thead>
  <tbody>
    <tr><td>the engines, an entry per export<br><code>models</code>, <code>default_engine</code></td><td>checks that this build can run one</td><td><b>prepare</b>: the entry loads</td></tr>
    <tr><td>the tensors: name, dtype, axes, role<br><code>inputs</code>, <code>outputs</code></td><td>checks the shapes, builds a slot each</td><td><b>process</b>: the blocks go through them</td></tr>
    <tr><td>the state, a pair<br><code>role: state</code>, <code>state_source</code></td><td>resolves the pair, allocates its buffers</td><td><b>process</b>: swapped after every run</td></tr>
    <tr><td>the model's own latency<br><code>latency</code> on the output</td><td>keeps it</td><td><b>prepare</b>: part of what the host is told</td></tr>
    <tr><td>the instances<br><code>max_instances</code></td><td>keeps it</td><td><b>prepare</b>: they load · <b>process</b>: in parallel</td></tr>
    <tr><td>the pool: threads, how they wait<br><code>ContextConfig</code></td><td>registers the settings</td><td><b>prepare</b>: the threads start</td></tr>
  </tbody>
</table>

Note:
- After create: everything is declared and checked, and nothing runs yet.
- Left, what we declared. The third column is all create did with it: it checked, it built the slots, and it allocated the state pairs' two buffers, the one memory that holds from here on.
- No model, no plan, no thread: all of that needs the contract, and the contract comes with the host's blocks.
- The right column says where each declaration takes effect. The next two rows show it.

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
- Everything here is in effect from prepare on; the next two chapters show it running.
- Next: prepare, when the host says which blocks it will deliver.
