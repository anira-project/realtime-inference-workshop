<h1>Step 1<br>Minimal C++ inference</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
    - First hands-on block. Everyone builds and runs before we talk about speed.

---

## Goal

- Load the model with LibTorch
- Run the test signal through it, block by block
- Get the same result as the model in Python

---

## What's given

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    {{FILE:forward_stateful.pt}}
    {{FILE:test_signal.h}}
    {{FILE:target_signal.h}}
  </div>
  <div class="given-column">
    <div class="given-label">Helpers</div>
    {{FILE:libtorch_engine.h}}
    {{FILE:support.h}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    {{FILE:main.cpp}}
    <code class="given-macro">WORKSHOP_MODEL_PATH</code>
  </div>
</div>

Note:
    - forward_stateful.pt: the model, TorchScript, graph + weights, 65 MB; stateful, the state lives inside it.
    - WORKSHOP_MODEL_PATH: set by CMake (-DWORKSHOP_MODEL=...), read in main.cpp as k_model.m_path.
    - test_signal.h / target_signal.h: compiled in, no audio files, no Python.

---

## `libtorch_engine.h`

```cpp
LibTorchEngine engine(path);          // Loads the model
engine.process(samples, num_samples); // One block, processed in place
engine.reset();                       // Clears the model's state
```

Every later step keeps these three methods.

Note:
    - What changes later is who calls process(), and from which thread.

---

## `libtorch_engine.h` — inside

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

## `test_signal.h` · `target_signal.h`

<div class="signal-pair">
  <div class="signal-label"><code>k_input_signal</code></div>
  {{WAVEFORM:steps/common/test_signal.h|blocks=2048|height=230}}
  <div class="signal-label"><code>k_target_output_signal</code></div>
  {{WAVEFORM:steps/common/target_signal.h|blocks=2048|height=230}}
</div>

Note:
    - Input: 220 Hz sine at half scale, 8 blocks of 2048 samples at 48 kHz. The dashed lines are the block boundaries.
    - Target: what the model made of it, in Python. The first block is nearly silent — the model's latency has not filled yet.

---

## `support.h`

```cpp
return report(output.data(), target.data(), output.size());
```

```plaintext
OK: max abs diff 3.51e-06, within 0.0001 of the reference.
```

Note:
    - Compares what you produced with the target and prints one line; the return value is main's exit code.

---

## Task

<div class="task-timer" data-minutes="5"></div>

Fill in the TODOs in `steps/01_minimal_inference/exercise/main.cpp`

```bash
cmake --preset release      # once
cmake --build --preset release --target step01_exercise
./build/bin/step01_exercise
```

<div class="pipeline">
  <div class="pipe-inputs">
    {{FILE:forward_stateful.pt}}
    {{FILE:test_signal.h}}
  </div>
  <div class="pipe-link"></div>
  <div class="pipe-run">
    {{ICON:cpp}}
    <div class="pipe-progress"><span></span></div>
  </div>
  <div class="pipe-link pipe-link-2"></div>
  <div class="pipe-output">
    {{WAVEFORM:steps/common/target_signal.h|blocks=2048|height=200}}
    <div class="pipe-label">output</div>
  </div>
  <div class="pipe-check"><span class="pipe-check-mark">✓</span><div class="pipe-label">target_signal.h</div></div>
</div>

---

## If you see this: perfect

```plaintext
OK: max abs diff 3.51e-06, within 0.0001 of the reference.
```

Note:
    - Not zero, and it should not be: the reference ran in ONNX Runtime in Python, our engine is LibTorch in C++. ~1e-6 is what "the same model" means across two runtimes.

---

## TODO 1 — create the engine

```cpp
std::unique_ptr<LibTorchEngine> engine;
try {
    engine = std::make_unique<LibTorchEngine>(k_model.m_path);
} catch (const std::runtime_error& error) { /* ... */ }
```

Once, **outside the loop**. Inside it, every block gets a fresh engine:

```plaintext
block 0   ok
block 1   wrong     <- state was thrown away
```

Slow, too: 65 MB read per block. **The engine lives as long as the stream.**

---

## TODO 2 — the size to process in

```cpp
const size_t process_size = static_cast<size_t>(k_model.m_input_size);  // 2048
```

Not a free choice: the export fixed the shape at **2048**. Anything else fails inside
LibTorch, with an error about tensors, not blocks:

```plaintext
process() failed: RuntimeError: The size of tensor a (6)
must match the size of tensor b (0) at non-singleton dimension 2
```

**The model sets the block size.**

---

## TODO 3 — run each block

```cpp
for (size_t i = 0; i < num_blocks; ++i) {
    engine->process(output.data() + i * process_size, process_size);
}
```

In place: afterwards `output` holds what the model made of the signal.

---

## What broke?

Nothing. The numbers are right.

But:
- how long does one `process()` take?
- who decides the block size in a real host?

At 48 kHz, 2048 samples is **42.7 ms** — per callback, not on average.

<!-- .slide: data-state="no-footer" -->

Note:
    - Straight into step 2: the host's block size.
