<h1>Step 1<br>Minimal C++ inference</h1>

<!-- .slide: data-state="no-header" -->

Note:
    - First hands-on block. Everyone builds and runs before we talk about speed.

---

## Goal

The test signal runs through the model in C++, and comes out within **1e-4** of what
the model produced in Python.

1. **Create the engine** — and decide *where* it goes
2. **Pick the size you process in** — and see whether the model agrees
3. **Run each block** through the engine

---

## What's given

```plaintext
models/forward_stateful.pt    the model: TorchScript, graph + weights, 65 MB
WORKSHOP_MODEL_PATH           its path, set by CMake → k_model.m_path
common/libtorch_engine.h      the engine, written for you
common/test_signal.h          220 Hz sine, 8 × 2048 samples
common/target_signal.h        what the model made of it, in Python
common/support.h              report(): compares and prints the result
exercise/main.cpp             three TODOs
```

- The model is **stateful**: each call continues where the last one ended
- TorchScript keeps that state inside the model: audio in, audio out
- No audio files, no Python: the signals are compiled in

---

## The engine

Three methods:

```cpp
LibTorchEngine engine(path);          // Loads the model
engine.process(samples, num_samples); // One block, processed in place
engine.reset();                       // Clears the model's state
```

Every later step keeps this interface. What changes is **who calls `process()`, and
from which thread**.

---

## Inside the engine — loading the model

```cpp
m_model = torch::jit::load(model_path);
m_model.eval();
```

- A TorchScript file carries the graph **and** the weights
- No model class to link against, no architecture in your C++ needed
- `eval()`: model in inference mode, no dropout, no batchnorm updates

---

## Inside the engine — a block

```cpp
const torch::NoGradGuard no_grad;                                               // 1
const auto input  = torch::from_blob(samples, {1, 1, length}, torch::kFloat32); // 2
const auto output = m_model.forward({input}).toTensor() .contiguous();          // 3
std::copy_n(output.data_ptr<float>(), num_samples, samples);                    // 4
```

1. **No autograd graph.** Inference only, so no allocations for it.
2. **`from_blob` wraps your buffer, doesn't copy it.** The tensor points at *your* buffer, so the buffer must outlive it. Shape is `{batch, channels, samples}`.
3. **Run.** `forward` also advances the model's **state**. `contiguous()` guarantees a flat layout to read from.
4. **Write back in place.** The caller's buffer now holds the output

---

## Task

<div class="task-timer" data-minutes="5"></div>

Fill in the TODOs in `steps/01_minimal_inference/exercise/main.cpp`

```bash
cmake --preset release                                        # once
cmake --build --preset release --target step01_exercise
./build/bin/step01_exercise
```

<div class="nn-flow task-flow">
  <div class="nn-node">model<small><code>forward_stateful.pt</code></small></div>
  <div class="nn-arrow">→</div>
  <div class="nn-node exercise">your C++<small><code>main.cpp</code> + engine</small></div>
  <div class="nn-arrow">←</div>
  <div class="nn-node">test signal<small>220 Hz sine</small></div>
  <div class="nn-arrow">→</div>
  <div class="nn-node check">output<small>vs. <code>target_signal.h</code>, 1e-4</small></div>
</div>

---

## If you see this: perfect

```plaintext
OK: max abs diff 3.51e-06, within 0.0001 of the reference.
```

Not zero, and it should not be: the reference ran in **ONNX Runtime, in Python**, our
engine runs in **LibTorch, in C++**.

&rarr; ~1e-6 is what "the same model" means across two runtimes.

The first block is nearly silent — the model's latency has not filled yet. It still
has to match.

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
