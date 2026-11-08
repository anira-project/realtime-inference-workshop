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
- I.e. our input audio is a streamed tensor named "audio_in". It's role is streamed because (it represents a continuous stream with a time axis)
- We define the channel number as 1 and the fixed time window of 2048

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
- Now recap how the state was handled in part 1
- It was declared as a size and two tensors in the engine, and that was all the code could say about it.
- That the state output feeds the state input was declared nowhere it was later implemented in the OnnxEngine::process call

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
- In anira state is just another role, next to streamed
- State tensors are declared as a pair:
- Here state_out feeds state_in.
- We can also see another information which we can provide in the anira ModelConfig:
- That is whether a streamed output tensor has a latency
- Appart from streamed and state anira also allows a third tensor role. Static tensors:
- Static tensors keep the same value in every inference run until overwritten and then keep this value for the next inference. I.e. this can be some parameter that the user can tweak (i.e. the decay time in a reverb emulation neural network model)
- The tensor specifications are known when the model is exported. So this is something that your data scientist can fill out for you ideally..

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
- Ok we have now defined our model
- What else did we declare in our ProcessorExample
- We have been declaring the worker thread
- In our example one thread per processor, and in step 8 one processor per plugin instance.
- But as Valentin already pointed out this does not scale well
- And apart from that we have been using a "dirty" hot spinning loop on the worker thread which is burning our full cpu compute of one thread while waiting for data.
- How is this handled by anira?

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
- In anira there is one configurable thread pool per process, and every instance queues into it.
- It is declared once, in the ContextConfig with the information of how many threads, and how they wait for work while idle.

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

<div class="policy-advice fragment"><span>We recommend BLOCKING.</span> One bounded wake instead of threads polling while idle.</div>

Note:
- anira has two waiting strategies, declared process wide
- SPIN_BACKOFF is the default: the audio thread only touches an atomic, and the idle pool with socalled exponential backoff
- With this technique, the balence between pooling for new data and leaving compute to other threads becomes bigger and bigger.
- first we have 4 hot spinning retries consuming all compute, then 32 more with a CPU pause hint of about 100 ns each, then a yield and a 100 µs sleep before every further retry
- So an idle thread picks up new work within about 100 µs.
- In BLOCKING mode, the WorkerThreads wait in moodycamel's semaphore: it first spins on the semaphore's count, up to 10,000 tries without a pause, then blocks on the OS semaphore until an enqueue wakes it.
- The price is on the audio thread: if a pool thread is blocked, enqueuing a new job to the thread pool makes one wake syscall.
- This is not strictly real-time safe but the idle thread can pick up work usually faster in the last idle phase, which is also why we recommend it in most use cases.

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
- And finally lets talk about memory. In step 8 the processor is a member of the plugin, and the engine a member of the processor: so there it is one engine per instance, fixed by the classes.
- Each instance loads the model in its prepare, so ten instances hold ten copies of the same model. Memory grows with the instances the host adds, like the threads did.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Memory</div>
<div class="when">→ <b>prepare</b>: the weights load</div>

## Ten instances, four copies.<br><span class="then">If the model has no state. Ours has: still ten.</span>

<table class="copies">
  <thead>
    <tr><th></th><th>Who loads the weights, at prepare</th><th>10 instances</th><th>1 instance</th></tr>
  </thead>
  <tbody>
    <tr><td>Part 1, step 8</td><td>every instance: its own session</td><td>10 copies</td><td>1 copy</td></tr>
    <tr><td>anira, with state<br>like ours</td><td>every handler: its own session</td><td>10 copies</td><td>1 copy</td></tr>
    <tr><td>anira, without state</td><td>the first handler: <code>max_instances</code> sessions, by default one per pool thread; every later one shares them</td><td><b>4 copies</b></td><td>4 copies</td></tr>
  </tbody>
</table>

<div class="cards-note">On the pool of four threads from the context file. A copy is one ONNX Runtime session with its own weights; anira shares only the environment and the session options.</div>

Note:
- anira loads the weights at prepare as well but How many copies are loaded depends on whether the model has state.
- With state, like ours: the state belongs to one stream, so every handler gets its own session at its prepare, and its inferences run one at a time, in order. One handler per plugin instance: ten instances, ten copies, the same as part 1.
- Without state: the first handler's prepare loads the model for the whole process. A model setting, max_instances, by default one per pool thread, then says how many engine instances that load creates
- With ONNX Runtime one session each, and every session reads the file and keeps its own copy of the weights. Every later handler shares them and loads nothing.
- So ten instances on our four threads would hold four copies, not ten.
- And similarly a single instance would hold four engines as well: This means that for a stateless model anira would allow multiple parallel inferences at once for one instance

---

<div class="tag">After create</div>

## Declared and checked.<br><span class="then">In effect from prepare on.</span>

<table class="declared">
  <thead>
    <tr><th>You declare</th><th>Create</th></tr>
  </thead>
  <tbody>
    <tr><td>the engines, an entry per export<br><code>models</code>, <code>default_engine</code></td><td>checks that this build can run one</td></tr>
    <tr><td>the tensors: name, dtype, axes, role<br><code>inputs</code>, <code>outputs</code></td><td>checks the shapes, builds a slot each</td></tr>
    <tr><td>the state, a pair<br><code>role: state</code>, <code>state_source</code></td><td>resolves the pair, allocates its buffers</td></tr>
    <tr><td>the model's own latency<br><code>latency</code> on the output</td><td>keeps it</td></tr>
    <tr><td>the instances<br><code>max_instances</code></td><td>keeps it</td></tr>
    <tr><td>the pool: threads, how they wait<br><code>ContextConfig</code></td><td>registers the settings</td></tr>
  </tbody>
</table>

Note:
- After create: everything is declared and checked, and nothing runs yet.
- Left, what we declared; right, all create did with it: it checked, it built the slots, and it allocated the state pair's two buffers.
- There is no model loaded, no ringbuffer allocated or no thread started yet
- Next up is prepare, where the host will provide us information about the blocks that it will deliver.
