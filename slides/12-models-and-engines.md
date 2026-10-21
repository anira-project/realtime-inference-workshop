<h1>Models and engines</h1>

Plans, stages, state

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - Three more concepts, same pattern: the step in part 1, then anira.

---

<div class="tag">Part 1 · Step 4</div>

## Every engine has its own API.<br><span class="then">And its own weight.</span>

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
    - Two engines, two classes, and the state handled differently in each. The plugin picked one at compile time.

---

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
  <div>Switching plans at runtime is one call; the latency covers the slowest</div>
</div>

Note:
    - Entries for engines the build does not have are skipped at prepare, so one file serves every build.
    - A plan is one model entry on one engine (and provider). The plan report at prepare says which plans exist and which one is selected.
    - Custom engine: a descriptor with load, prepare, process; the hand-written network from step 4 fits here.
    - Loaded models are pooled: ten instances of the same file share one loaded engine object per plan.

---

<div class="tag">Part 1 · Steps 2 and 7</div>

## Pre- and post-processing,<br><span class="then">inside the processor, by hand.</span>

```cpp
m_input.push(samples, num_samples);                 // host block in
while (m_input.available() >= k_model.m_input_size) { … try_enqueue … }   // model block out
…
m_output.pop(m_wet_block.data(), num_samples);      // model block in, host block out
```

Note:
    - Ring in, model block out, model block in, ring out: that is pre- and post-processing, and ours knew only one shape, audio to audio.

---

<div class="tag">anira · Stages</div>

## A stage before and after the model.<br><span class="then">Three kinds of thread, each with its calls.</span>

<div class="threads">
  <div class="threads-label">main</div>
  <div class="threads-lane"><span>from_file</span><span>create</span><span>prepare</span><span>get_latency</span><span>destroy</span></div>
  <div class="threads-label">audio</div>
  <div class="threads-lane audio"><span>process</span><span class="stage">pre_process</span><span class="stage">post_process</span></div>
  <div class="threads-label">pool</div>
  <div class="threads-lane pool"><span class="stage">before_inference</span><span class="engine">engine</span><span class="stage">after_inference</span></div>
</div>

<div class="cards-note">The default stage is step 2: window out of the ring, result back in. You write one only for another shape.</div>

Note:
    - A stage is the 3.x form of the PrePostProcessor: one per pipeline, with up to four phases. pre_process and post_process run on the audio thread when the block is formed and consumed; before_inference and after_inference on the pool thread around the engine call.
    - The default stage does what step 2 did: pop a window from the ring into the model tensor, push the result back. A receptive-field model declares window and overlap, and the default stage fills the history.
    - The thread tags are part of the API: every entry says which thread may call it, the way CLAP does.

---

<div class="tag">Part 1 · ONNX</div>

## 63,488 floats of state,<br><span class="then">carried by hand.</span>

```cpp
// Carry the new state over to the next call. Hidden in LibTorch, ours here.
std::copy_n(outputs[1].GetTensorData<float>(), m_state.size(), m_state.begin());
```

Note:
    - OnnxEngine from step 5: the state tensor out, copied into the state tensor in, every call. 63,488 floats, one copy per block, and the ordering of calls is ours to guarantee.

---

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

<div class="tag">Part 1 · Steps 3 and 5</div>

## We measured and checked by hand.

| step 3 | step 5 |
|---|---|
| mean, p95, p99, max | RTSan on both engines |
| the max decides | `[[clang::nonblocking]]` on the callback |

Note:
    - A benchmark we wrote, a sanitizer we ran once. Both have to be repeated for every model, every engine, every machine.

---

<div class="tag">anira · Safety</div>

## Measured at warm-up, checked at <code>prepare</code>.<br><span class="then">The entries are <code>nonblocking</code>.</span>

<div class="statement-points">
  <div>Warm-up runs the model on the thread that calls <code>prepare</code>, until its time is stable or for a fixed count; the budget is the worst case seen</div>
  <div>RTSan builds in CI: <code>process</code>, <code>push</code>, <code>pop</code> and <code>reset</code> carry <code>ANIRA_NONBLOCKING</code></div>
  <div>A benchmark fixture for your own model: real-time factor and underruns</div>
</div>

Note:
    - Warm-up blocks prepare, so prepareToPlay takes a few model runs longer; {"fixed": n} in the contract bounds it.
    - Step 3's fixture is anira's: ProcessBlockFixture over Google Benchmark, parameterised by block size, engine and model.
    - Step 5's promise is in the headers: every audio-thread entry is tagged, and the library's own CI compiles with -fsanitize=realtime and runs the tests.

---

<div class="tag">Take away</div>

## Declare, do not implement.

<div class="statement-points">
  <div>Engines are entries in a file; the plan is picked at runtime</div>
  <div>Pre- and post-processing is a stage with a thread per phase</div>
  <div>State is a role: declared once, carried by the library</div>
</div>

Note:
    - Next: the whole picture, and the three places in a plugin where all of this is called.
