<h1>Step 1<br>Minimal C++ inference</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
    - First hands-on block. Everyone builds and runs before we talk about speed.

---

<div class="tag">Goal</div>

## Run the model in C++<br><span class="then">and reproduce the Python output.</span>

<div class="statement-points">
  <div><small>1</small>Load the model with LibTorch</div>
  <div><small>2</small>Run the test signal through it, block by block</div>
  <div><small>3</small>Compare with what the model made in Python</div>
</div>


---

<div class="tag">What's given</div>

## 3 assets, 1 helper, 1 file to edit.

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">models/ · common/assets/</div>
    {{FILE:forward_stateful.pt}}
    {{FILE:test_signal.h}}
    {{FILE:target_signal.h}}
  </div>
  <div class="given-column">
    <div class="given-label">Helpers</div>
    <div class="given-folder">common/helpers/</div>
    {{FILE:libtorch_engine.h}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">01_minimal_inference/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
    - forward_stateful.pt: the model, TorchScript, graph + weights, 65 MB; stateful, the state lives inside it.
    - WORKSHOP_MODEL_PATH: set by CMake (-DWORKSHOP_MODEL=...), read in main.cpp as k_model.m_path.
    - support.h (report_line, k_tolerance) is plumbing for the checks and stays off the slides.
    - test_signal.h / target_signal.h: compiled in, no audio files, no Python.

---

<div class="tag">Assets · <code>test_signal.h</code> · <code>target_signal.h</code></div>

## Input: a sine.<br><span class="then">Reference: the model's output in Python.</span>

<div class="signal-pair">
  <div class="signal-label"><code>k_input_signal</code></div>
  {{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=230|from=2|to=4}}
  <div class="signal-label"><code>k_target_output_signal</code></div>
  {{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=230|from=2|to=4}}
</div>

Note:
    - Input: 220 Hz sine at half scale, 8 blocks of 2048 samples at 48 kHz; shown here: blocks 3 and 4, the dashed line between them is a block boundary.
    - Target: what the model made of it, in Python. The first block is nearly silent — the model's latency has not filled yet.

---

<div class="tag">Helpers · <code>libtorch_engine.h</code></div>

## The engine interface has three methods.<br><span class="then">Later steps reuse it unchanged.</span>

```cpp
LibTorchEngine engine(path);          // Loads the model
engine.process(samples, num_samples); // One block, processed in place
engine.reset();                       // Clears the model's state
```

Note:
    - What changes later is who calls process(), and from which thread.

---

<div class="tag">Helpers · <code>libtorch_engine.h</code> — inside</div>

## Load once, then one block at a time.

```cpp
// Constructor
m_model = torch::jit::load(model_path);
m_model.eval();

// process()
const torch::NoGradGuard no_grad;
const auto input  = torch::from_blob(samples, {1, 1, length}, torch::kFloat32);
const auto output = m_model.forward({input}).toTensor().contiguous();
std::copy_n(output.data_ptr<float>(), num_samples, samples);
```

Note:
    - load: a TorchScript file carries graph and weights; no model class in C++. eval(): inference mode.
    - NoGradGuard: no autograd graph, no allocations for it.
    - from_blob wraps your buffer, no copy — the buffer must outlive the tensor. Shape {batch, channels, samples}.
    - forward also advances the model's state; contiguous() guarantees a flat layout to read from.
    - copy_n writes back in place.

---

## Task

<div class="task-timer" data-minutes="5"></div>

Fill in the TODOs in `steps/01_minimal_inference/exercise/main.cpp`

```bash
cmake --preset release      # once
cmake --build --preset release --target step01_exercise
./build/bin/step01_exercise
```

<div class="run" style="grid-template-columns: 1fr 1.5fr 1.5fr">
  <div class="run-step" data-seconds="2">
    <div class="run-title"><span>1</span>Create the engine</div>
    <div class="run-visual">
      <div class="run-file">{{FILE:forward_stateful.pt}}</div>
      <div class="run-engine">{{ICON:cpp}}<small>LibTorch</small></div>
    </div>
  </div>
  <div class="run-step" data-seconds="2.2">
    <div class="run-title"><span>2</span>Pick the block size</div>
    <div class="run-visual run-wave">
      {{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=120|from=2|to=6}}
      <div class="run-cuts"><i></i><i></i><i></i></div>
      <div class="run-size">? samples</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3.4">
    <div class="run-title"><span>3</span>Run each block</div>
    <div class="run-visual run-wave">
      <div class="run-target">{{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=120|from=2|to=6}}</div>
      <div class="run-reveal">{{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=120|from=2|to=6}}</div>
      <div class="run-window"></div>
      <div class="run-ok">✓ matches the reference</div>
    </div>
  </div>
</div>

---

<div class="tag">Solution</div>

## TODO 1 · Create the engine

```cpp
std::unique_ptr<LibTorchEngine> engine;
try {
    engine = std::make_unique<LibTorchEngine>(k_model.m_path);
} catch (const std::runtime_error& error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 2;
}
```

Note:
    - Outside the loop. Created inside, block 1 is already wrong: the state was thrown away.
    - Slow, too: 65 MB read per block. The engine lives as long as the stream.

---

<div class="tag">Solution</div>

## TODO 2 · Pick the block size

```cpp
const size_t process_size = static_cast<size_t>(k_model.m_input_size);  // 2048
```

<div class="run-visual run-wave run-solved">
  {{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=120|from=2|to=6}}
  <div class="run-cuts"><i></i><i></i><i></i></div>
  <div class="run-size"><span class="was">? samples</span><span class="is">2048 samples</span></div>
</div>

Note:
    - The model sets it, fixed at export.
    - Anything else fails inside LibTorch, with an error about tensors, not blocks:
      "The size of tensor a (6) must match the size of tensor b (0) at non-singleton dimension 2".

---

<div class="tag">Solution</div>

## TODO 3 · Run each block

```cpp
for (size_t i = 0; i < num_blocks; ++i) {
    engine->process(output.data() + i * process_size, process_size);
}
```

Note:
    - In place: afterwards output holds what the model made of the signal.

---

<div class="tag">Done</div>

## Congratulations!<br><span class="then">You integrated an AI model in C++.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
    - Same output as in Python. Whether it is fast enough is still open.
    - Straight into step 2: the host's block size.
