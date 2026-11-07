<h1>Create</h1>

What you declare: the model and the pool. Nothing runs yet.

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- The first row of the map: the creation of the InferenceHandler, once, on the main thread, in the plugin's constructor.
- Two inputs, the model config and the context; one output, the handler. We declare everything it needs: the engines, the tensors and the state in the ModelConfig, and the thread pool in the optional ContextConfig.
- Create checks the model config: the tensors, the axes, and which engines and providers this build can run. Then it builds the handler's slots, one per tensor, the state pairs among them. It opens no model file: the models, the plans and the pool's threads all come at prepare.
- As in step 8: there the constructor makes the processor, with the model, and its engine, which load nothing, and the processor's prepare loads the model. anira's create does a little more, it checks the configs and builds the slots, but it loads no model either.
- One rule for the whole chapter: create declares and checks. Nothing we declare here takes effect before prepare or process, and that is where we look at it; every anira slide says which, top right.
- Nothing in this row runs on the audio thread, so all of it may read files and allocate.
- We start as in part 1, with what we wrote by hand.

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
- First the engines. In part 1 we wrote one class for one engine, ONNX Runtime, by hand: it owns the session and the tensors, and later loads and runs the model.
- The engine was a decision of the code: another engine, LibTorch, ExecuTorch, LiteRT or your own, would be another class like it, with its own API and its own way of handling the state. The plugin is built for one engine, at compile time.
- In step 8 the ProcessorExample owns this class: made with it, in the plugin's constructor, where it loads nothing, and loaded in its prepare.

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
- anira brings the engines: LibTorch, ONNX Runtime, ExecuTorch, LiteRT and TensorFlow Lite, prebuilt, static or shared, compiled in at build time.
- Each one is an adapter behind the same interface: what we wrote by hand as OnnxEngine, once per engine.
- Or bring your own: a custom engine is your own code behind the same four calls, descriptor, load, prepare and process, so a class like ours from part 1 fits there. Its size is whatever your engine weighs.
- A custom engine mirrors step 8: you make it and add it to the pipeline before create, as our ProcessorExample makes its OnnxEngine when the plugin constructs it. anira calls its load at prepare, like our load in the processor's prepare, and its process on a pool thread.
- They cost very different amounts to ship: LibTorch 318 MB, ONNX Runtime 26 MB, ExecuTorch 17 MB, LiteRT 11 MB, TensorFlow Lite 5 MB.
- Measured: the libraries unpacked from the anira-project/backends releases, macOS arm64 (LibTorch 2.12.0, ONNX Runtime 1.30.0, ExecuTorch 1.3.1, LiteRT 2.2.0, TensorFlow Lite 2.17.0). LibTorch ships only shared, ExecuTorch only static; less of a static library ends up in a linked plugin.
- anira's public interface is a C ABI, and everything else is hidden: only the anira_ symbols are exported. So two plugins with different versions of anira, next to a host that ships its own ONNX Runtime, do not clash.
- Built in or your own, every engine loads its model at prepare; create only checks that this build can run it. An entry for an engine the build does not have is skipped.

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
- How to configure it: you list them in one model config, one entry per engine, each with its own export of the model.
- Here four: ONNX Runtime, LibTorch, ExecuTorch and LiteRT, each loading its own file.
- default_engine says which one runs. You can benchmark the model on each engine, from the same config.
- Every entry has to take the same tensors, state included.
- An entry for an engine the build does not have is skipped, so one file serves every build.
- Create checks the file's structure: the tensors, the axes, and which entries this build can run. The plans are built at prepare, when the models load.

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
- For our example we use ONNX Runtime only, to keep it simple: one entry, one export.
- The workshop builds only ONNX Runtime, so another entry would be skipped anyway.
- And our model.pt keeps its state inside: next to this entry it would need an export with the state in and out.

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
- Next, the tensors. Part 1 had a model config too: k_model, five facts typed by hand from the export metadata, step by step.
- The path, the model's block of 2048 samples, the state of 63,488 floats, the rate it was trained at, and its own delay of 2224 samples.
- The processor takes it at construction and keeps it, as anira's create takes the model config. What the processor does with it comes at prepare.
- Everything else the code assumes: every buffer is a float vector, one channel, one model block, and Ort::Value exists only inside OnnxEngine.
- A model with an int input, a second channel or an image does not fit this code.
- The sample rate is the one fact anira's model file does not take: the host's rate comes with the contract at prepare, and whether the model fits it stays the plugin's call, step 9's ask_unless_44k.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Tensors</div>
<div class="when">→ <b>process</b>: the blocks go through the slots</div>

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

```cpp
// once per process; the defaults: cores / 2, SPIN_BACKOFF
anira::ContextConfig config;
config.threads(4, ANIRA_WAIT_BLOCKING);
anira::Context context{config};
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
