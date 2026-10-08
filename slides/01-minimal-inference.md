<h1>Step 1<br>Minimal C++ inference</h1>

<!-- .slide: data-state="no-header" -->
<!-- part: Real-time inference -->

Note:
    - First hands-on block. Everyone builds and runs before we talk about speed.

---
##  Goal
   
1. Load the exported model with LibTorch
2. Run a test signal through it, block by block
3. Match what the model produced in Python: within 1e-4

---

## The current state

- The model is trained and exported - we are not touching either
- It is **stateful**: each call continues where the last one ended
- Two exports, same weights:
  - **TorchScript** (`.pt`) — state inside the model, audio in, audio out
  - **ONNX** — state in and out, carried by the caller

&rarr; For today: TorchScript in C++, with LibTorch.

---

## What's given

```
models/forward_stateful.pt      the model: graph + weights, 65 MB
common/libtorch_engine.h        the engine, written for you
common/test_signal.h            220 Hz sine, 3 x 2048 samples
common/target_signal.h          what the model made of it, in Python
common/support.h                comparison and reporting
exercise/main.cpp               three TODOs
```

No audio files, no Python: the signals are compiled in c++.

---

## The engine

The engine has three methods.

```cpp
LibTorchEngine engine(path);          // Loads the model
engine.process(samples, num_samples); // One block, processed in place
engine.reset();                       // Clears the model's state
```

Later steps keep this interface and change **who calls
`process()`, and from which thread**. (was willst du damit sagen?)  

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

<!-- ## Inside the engine — a block

```cpp
const torch::NoGradGuard no_grad; // No autograd graph, no allocations for it

const auto input = torch::from_blob(samples, {1, 1, length}, torch::kFloat32);
const auto output = m_model.forward({input}).toTensor().contiguous();

std::copy_n(output.data_ptr<float>(), num_samples, samples);
```

- `from_blob` wraps your buffer — no copy, so it has to outlive the tensor
- `{batch, channels, samples}`, even when two of them are 1
- `NoGradGuard`: no autograd graph, no allocations for it -->

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

## Task - 5 minutes

1. **Create the engine** — and decide *where* it goes
2. **Pick the size you process in** — and see if the model agrees
3. **Run each block** through it

then complile and run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/bin/step01_exercise
```


---

## What we check

```
OK: max abs diff 2.21e-06, within 0.0001 of the reference.
```

Not zero, and it should not be: the reference ran in **ONNX Runtime, in Python**, our engine runs in **LibTorch, in C++.**

&rarr; ~1e-6 is what "the same model" means across two runtimes.


The first block is nearly silent — the model's latency has not filled yet. It
still has to match.
---

## Mistakes worth making

**1. The engine inside the loop**

```
block 0   ok
block 1   wrong     <- state was thrown away
block 2   wrong
```
 
- Every `process()` on a fresh engine starts from silence.
- Block 0 cannot tell the difference. Everything after it can.
- Very slow: reads 65 MB file per block.

**Rule:** the engine's lifetime is the stream's lifetime.

---

## Mistakes worth making

**2. A block size the export has never seen before**
The export fixed the shape at **2048**. Any other size fails inside LibTorch,
with an error that mentions tensors, not blocks.

```
process() failed: RuntimeError: The size of tensor a (6)
must match the size of tensor b (0) at non-singleton dimension 2
```

**Rule:** the model sets the block size. 

---

## What broke?

Nothing. The numbers are right.

But: 
- how long does one `process()` take?
- who decides the block size in a real host?

<!-- one forward pass takes *a certain* amount of time, and nobody measured it. -->

At 48 kHz, 2048 samples is **42.7 ms** — per callback, not on average.

<!-- .slide: data-state="no-footer" -->

Note:
    - Straight into step 5: mean vs. worst case.
