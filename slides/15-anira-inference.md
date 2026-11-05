<h1>Step 9<br>The model with anira</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- Back to coding.
- In Step 9 we take the model from part 1 and run it through anira.
- We use the same model and the same check, but this time Steps 3, 6 and 7 are not written by hand.

---

<div class="tag">Motivation</div>

## The state loop goes.<br><span class="then">Declared once, carried by the library.</span>

<div class="state-demo">
  <div class="state-row was">
    <div class="state-label">step 2</div>
    <code>std::copy_n(outputs[1].GetTensorData&lt;float&gt;(), m_state.size(), m_state.begin());</code>
  </div>
  <div class="state-row is">
    <div class="state-label">step 9</div>
    <code>"role": "state", "state_source": "state_out"</code>
  </div>
</div>

Note:
- Remember the line in Step 2 where we copied the state back by hand?
- In anira it is one line in the model file: this input is a state, and its source is that output.
- The ring buffers, the worker and the latency go the same way.

---

<div class="tag">Goal</div>

## Same output, same check.<br><span class="then">Two files, three calls.</span>

<div class="statement-points">
  <div><small>1</small>The model file: tensors, roles, the state pair</div>
  <div><small>2</small>The contract: budget, warm-up, on a miss</div>
  <div><small>3</small><code>prepare</code>, <code>process</code>, and the reference shifted by <code>get_latency()</code></div>
</div>

Note:
- The goal: the same output and the same check as in part 1, now with two files and three calls.
- First, the model file: which tensors the model has, what role each one plays, and which output feeds which state.
- Second, the contract: the time budget, the warm-up, and what happens on a miss.
- Third, the calls in main.cpp, and the check against the reference, shifted by the latency anira reports.

---

<div class="tag">Given</div>

## 1 new library, 2 files to edit.

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">common/assets/</div>
    {{FILE:model_forward.onnx}}
    {{FILE:test_signal.h}}
    {{FILE:target_signal.h}}
  </div>
  <div class="given-column many">
    <div class="given-label">Helpers</div>
    <div class="given-folder">09_anira/exercise/ · CMake</div>
    {{FILE:contract.json|new}}
    {{FILE:anira|new}}
    {{FILE:host.h}}
    {{FILE:support.h}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">09_anira/exercise/</div>
    {{FILE:model.json|new}}
    {{FILE:main.cpp}}
  </div>
</div>

Note:
- CMake fetches anira, pinned to a v3 pre-release with ONNX Runtime built in.
- The contract is complete.
- The model file is given with gaps: the tensors are named, and their shapes and roles are your task.
- Your other file is main.cpp.

---

<div class="tag">Helpers: <code>model.json</code></div>

## What the model is.

```json
{
  "models":  [ { "engine": "onnxruntime", "path": "../../common/assets/models/rave/model_forward.onnx" } ],
  "inputs":  [ { "name": "audio",     "role": "streamed",
                 "axes": [["batch",1],["channel",1],["time",2048]],
                 "window": { "min": 2048, "max": 2048 } },
               { "name": "state_in",  "role": "state", … } ],
  "outputs": [ { "name": "audio_out", "role": "streamed", …, "latency": 2224 },
               { "name": "state_out", "role": "state", … } ]
}
```

Note:
- We provide this model.json; it describes the model to anira.
- Under models: the engine, ONNX Runtime, and the path to our ONNX file.
- Under inputs: the model's input tensors. The first is audio, and it is streamed: audio comes in continuously, and anira cuts it into blocks for the model.
- Its axes are its shape, each with a name: batch 1, channel 1, time 2048, so a tensor of 1 by 1 by 2048.
- The window is the same number as our input size, 2048: how many samples one inference takes.
- If the model needed overlapping windows or a receptive field, the extra past samples would be added here.
- The second input is state_in, with the role state: no time axis, it is carried from one call to the next.
- The outputs mirror the inputs: audio_out, streamed, with the same shape and window, and state_out, the new state.
- On audio_out sits a number you know from Step 7: the model's own latency of 2224 samples, declared on the output it belongs to.

---

<div class="tag">Helpers: <code>contract.json</code></div>

## How it runs.

```json
{ "hard": { "budget": "measured", "warmup": "until_stable", "on_miss": "bypass" } }
```

Three lines. The geometry comes from the host, at `prepare`.

Note:
- The contract is three lines.
- We have a hard contract because this is a real-time stream: the host calls us with fixed blocks, every block has a deadline, and nothing may wait. For jobs without a stream, like images, there is an async contract.
- The budget is measured: anira benchmarks the model at prepare, on this machine, with this engine. That is Step 4, done by the library.
- The warm-up runs until the times are stable, before prepare returns. We saw why in Step 4: the first calls took much longer, about 60 and 40 ms instead of 7.5, so we make them before the audio starts.
- And bypass on a miss: a block whose result is late goes out as the input block, not as zeros like in Step 7.
- The host's blocks come in at prepare.

---

<div class="tag">Task: <code>model.json</code> · <code>main.cpp</code></div>

## Task

<div class="task-timer" data-minutes="10"></div>

Fill in the TODOs in `steps/09_anira/exercise/model.json` and `main.cpp`

```bash
cmake --build --preset release --target step09_exercise
./build/bin/step09_exercise
```

<div class="run">
  <div class="run-step" data-seconds="2.4">
    <div class="run-title"><span>1</span>Declare the tensors</div>
    <div class="run-visual run-setup">
      <div class="run-file">{{FILE:model.json}}</div>
      <div class="run-note">audio · state · audio · state</div>
    </div>
  </div>
  <div class="run-step" data-seconds="2.4">
    <div class="run-title"><span>2</span>Pair the state</div>
    <div class="run-visual run-setup">
      <div class="run-note"><code>state_out</code> → <code>state_in</code></div>
    </div>
  </div>
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>3</span>Prepare and process</div>
    <div class="run-visual run-worker">
      <div class="q small one"><div class="q-slot"><b></b></div></div>
      <div class="mini-model">anira</div>
      <div class="q small one out"><div class="q-slot"><b></b></div></div>
      <div class="run-ok">✓ matches the reference, shifted by <code>get_latency()</code></div>
    </div>
  </div>
</div>

Note:
- You have ten minutes.
- Fill in the TODOs in model.json and main.cpp, then build step09_exercise and run it.
- You are done when it says it matches the reference.

---

<div class="tag">Solution</div>

## TODO 1 · Declare the tensors

```json
"inputs": [
  { "name": "audio", "dtype": "float32", "role": "streamed",
    "axes": [["batch",1],["channel",1],["time",2048]],
    "window": { "min": 2048, "max": 2048 } },
  { "name": "state_in", "dtype": "float32", "role": "state",
    "axes": [["batch",1],["any",63488]], "state_source": "state_out" }
],
"outputs": [
  { "name": "audio_out", "dtype": "float32", "role": "streamed",
    "axes": [["batch",1],["channel",1],["time",2048]],
    "window": { "min": 2048, "max": 2048 }, "latency": 2224 },
  { "name": "state_out", "dtype": "float32", "role": "state",
    "axes": [["batch",1],["any",63488]] }
]
```

Note:
- Here are the four tensors.
- Audio in and out have a time axis and a window of 2048.
- The state has no time axis: it is one flat block of 63,488 floats, the same state we copied by hand in Step 2. Its size was given.
- If a size is wrong, prepare refuses the file and names the tensor.

---

<div class="tag">Solution</div>

## TODO 2 · Pair the state

```json
{ "name": "state_in", "role": "state", "axes": [["batch", 1], ["any", 63488]],
  "state_source": "state_out" }
```

Note:
- The pairing is one field, state_source.
- Try leaving it out once: create refuses the file, because nothing feeds that state.
- With the pair in place, anira knows each call depends on the one before.
- So it never runs two blocks of this model at the same time, and it keeps them in order.

---

<div class="tag">Solution</div>

## TODO 3 · Prepare and process

```cpp
const auto model = anira::ModelConfig::from_file(step_dir + "/model.json");
auto contract    = anira::ContractHandle::from_file(step_dir + "/contract.json");
contract.hard_host_blocks(1, max_block_size, sample_rate);   // any block up to the host's largest

anira::Pipeline pipeline;
pipeline.add_inference(model);
anira::InferenceHandler handler(context, pipeline);
handler.prepare(contract);                          // warm-up runs here
const size_t latency = handler.get_latency(0);      // output slot 0

// In the audio callback: one call, in place
status = handler.process(block, 0, block, 0);
```

Note:
- Loading, the pipeline, the handler and prepare were given, all from anira.hpp; the warm-up runs inside prepare.
- First blank: give the contract the host's blocks, any size from 1 up to its largest, and its sample rate.
- Second: ask for the latency of output 0. Here it is 6320: the host may call with any block up to 512, so the latency covers the worst one, whatever the budget.
- Third: in the audio callback, one call over the block, in place: the same block is input and output.

---

<div class="tag">Done</div>

## Steps 3, 6 and 7,<br><span class="then">in two files.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
- That is Steps 3, 6 and 7 in two files.
- There is no ring buffer, no worker, no queue, no state copy and no latency arithmetic in main.cpp, and the check from Step 7 passes.
- Next we take the model apart.
