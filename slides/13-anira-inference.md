<h1>Step 9<br>The model with anira</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
    - Steps 2, 6 and 7, replaced by two files and three calls. Same model, same check.

---

<div class="tag">Motivation</div>

## The state loop goes.<br><span class="then">Declared once, carried by the library.</span>

<div class="state-demo">
  <div class="state-row was">
    <div class="state-label">step 5</div>
    <code>std::copy_n(outputs[1].GetTensorData&lt;float&gt;(), m_state.size(), m_state.begin());</code>
  </div>
  <div class="state-row is">
    <div class="state-label">step 9</div>
    <code>"role": "state", "state_source": "state_out"</code>
  </div>
</div>

Note:
    - Everything OnnxEngine did by hand is one line in the model file. The ring buffers, the worker and the latency go the same way.

---

<div class="tag">Goal</div>

## Same output, same check.<br><span class="then">Two files, three calls.</span>

<div class="statement-points">
  <div><small>1</small>The model file: tensors, roles, the state pair</div>
  <div><small>2</small>The contract: budget, warm-up, on a miss</div>
  <div><small>3</small><code>prepare</code>, <code>process</code>, and the reference shifted by <code>get_latency()</code></div>
</div>

---

<div class="tag">Given</div>

## 1 new library, 2 files to edit.

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">models/ · common/assets/</div>
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
    - anira is fetched by CMake, pinned to the v3 beta tag, with ONNX Runtime built in. support.h gains report_with_latency: the step 7 comparison, shifted by one number. No LibTorch in this step: the plugin's choice from step 8.
    - model.json is given with gaps: the tensors are named, shapes and roles are the task. contract.json is complete.

---

<div class="tag">Helpers: <code>model.json</code></div>

## What the model is.

```json
{
  "models":  [ { "engine": "onnxruntime", "path": "../../models/model_forward.onnx" } ],
  "inputs":  [ { "name": "audio",     "role": "streamed",
                 "axes": [["batch",1],["channel",1],["time",2048]],
                 "window": { "min": 2048, "max": 2048 } },
               { "name": "state_in",  "role": "state", … } ],
  "outputs": [ { "name": "audio_out", "role": "streamed", …, "latency": 2224 },
               { "name": "state_out", "role": "state", … } ]
}
```

Note:
    - Canonical names are ours; the ONNX names are bound by position here, since the file's order matches. A "tensors" record on the entry would bind by name.
    - "latency": 2224 is the number from step 7, on the output it belongs to.

---

<div class="tag">Helpers: <code>contract.json</code></div>

## How it runs.

```json
{ "hard": { "budget": { "ms": 8.2 }, "warmup": { "fixed": 3 }, "on_miss": "zeros" } }
```

Three lines. The geometry comes from the host, at `prepare`.

Note:
    - budget: the number from step 3, the worst case we are willing to promise. "measured" would derive it during warm-up; this build of the beta wants it explicit.
    - warmup fixed 3: three runs on the calling thread before prepare returns.
    - on_miss zeros: what step 7's wet debt did.

---

<div class="tag">Task: <code>model.json</code> · <code>main.cpp</code></div>

## Task

<div class="task-timer" data-minutes="15"></div>

Fill in the TODOs in `steps/09_anira/model.json` and `exercise/main.cpp`

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
    - A state spec has no time axis and no window. Its shape is checked against the file at prepare: a wrong extent is refused by name.

---

<div class="tag">Solution</div>

## TODO 2 · Pair the state

```json
{ "name": "state_in", "role": "state", "axes": [["batch", 1], ["any", 63488]],
  "state_source": "state_out" }
```

Note:
    - Without the source, the input is a state nobody feeds: prepare refuses it. With it, the model is stateful: one inference in flight, in order.
    - Leave it out on purpose once and run: block 0 matches, block 1 does not. The same failure as a fresh engine per block in step 1.

---

<div class="tag">Solution</div>

## TODO 3 · Prepare and process

```cpp
const auto model = anira::ModelConfig::from_file(step_dir + "/model.json");
auto contract    = anira::ContractHandle::from_file(step_dir + "/contract.json");
contract.hard_geometry(k_host.m_block_size, k_host.m_block_size, k_host.m_sample_rate);

const anira::Pipeline pipeline{anira::stage::Inference(model)};
anira_handler_create(context.native(), pipeline.native(), &handler.m_handle, &error);
anira_handler_prepare(handler.m_handle, contract.native(), &error);  // warm-up runs here
latency = anira_handler_get_latency(handler.m_handle, 0);             // output slot 0
```

Note:
    - Every C call returns a status and fills `error`; the solution throws on anything but ANIRA_OK.
    - The audio callback is given: one tensor over the block, anira_handler_process in place, ANIRA_MISSED counted.
    - The check is step 7's: shifted by the latency less the model's own 2224, which the reference already carries.
    - Expected: 4272 samples at a host block of 512, max abs diff 3.9e-07. A few late callbacks on a busy machine are listed and left out.

---

<div class="tag">Done</div>

## Steps 2, 6 and 7,<br><span class="then">in two files.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
    - No ring buffer, no worker, no queue, no state copy, no latency arithmetic in main.cpp. The check is the one from step 7, and it passes with 2048 samples less latency.
    - Next: the model taken apart, encoder and decoder as two handlers.
