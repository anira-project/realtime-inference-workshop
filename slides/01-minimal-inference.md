<h1>Step 1<br>Minimal C++ inference</h1>

Load the model, run it, prove it is the same model

<!-- .slide: data-state="no-header" -->

Note:
    - First hands-on block. Everyone builds and runs before we talk about speed.

---

## Where we are

- The model is trained and exported — we are not touching either
- It is **stateful**: each call continues where the last one ended
- Two exports, same weights:
  - **TorchScript** (`.pt`) — state inside the model, audio in, audio out
  - **ONNX** — state in and out, carried by the caller

Today: the TorchScript one, in C++, with LibTorch.

---

## What you get

```
models/forward_stateful.pt      the model: graph + weights, 65 MB
helpers/libtorch_engine.h       the engine, written for you
helpers/test_signal.h           220 Hz sine, 3 x 2048 samples
helpers/target_signal.h         what the model made of it, in Python
helpers/support.h               comparison and reporting
exercise/main.cpp               three TODOs
```

No audio files, no Python: the signals are compiled in.

---

## The engine

```cpp
LibTorchEngine engine(path);          // Loads the model, throws if it cannot
engine.process(samples, num_samples); // One block, in place
engine.reset();                       // Clears the state, like prepare()
```

Three methods. Later steps keep this interface and change **who calls
`process()`, and from which thread**.

---

## Inside it — loading

```cpp
m_model = torch::jit::load(model_path);
m_model.eval();
```

- A TorchScript file carries the graph **and** the weights
- No model class to link against, no architecture in your C++
- `eval()`: inference mode, no dropout, no batchnorm updates

---

## Inside it — one block

```cpp
const torch::NoGradGuard no_grad;

const auto input = torch::from_blob(samples, {1, 1, length}, torch::kFloat32);
const auto output = m_model.forward({input}).toTensor().contiguous();

std::copy_n(output.data_ptr<float>(), num_samples, samples);
```

- `from_blob` wraps your buffer — no copy, so it has to outlive the tensor
- `{batch, channels, samples}`, even when two of them are 1
- `NoGradGuard`: no autograd graph, no allocations for it

---

## Your job

1. **Create the engine** — and decide *where* it goes
2. **Pick the size you process in** — and see if the model agrees
3. **Run each block** through it

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/bin/step01_exercise
```

Note:
    - Let them hit the wrong block size themselves. That is the point of TODO 2.

---

## Two mistakes worth making

**The engine inside the loop**

Every block starts from a cleared state. Block 0 is right, everything after it
is wrong — and it is slow, because that is a 65 MB file per block.

**A block size the export never saw**

```
process() failed: RuntimeError: The size of tensor a (6)
must match the size of tensor b (0) at non-singleton dimension 2
```

The export fixed it: **2048 samples** per forward pass.

---

## Done right

```
OK: max abs diff 2.21e-06, within 0.0001 of the reference.
```

Not zero, and it should not be: the reference ran in **ONNX Runtime, in
Python**. Agreeing to ~1e-6 is what "the same model" means across two runtimes.

The first block is nearly silent — the model's latency has not filled yet. It
still has to match.

---

## What broke?

Nothing. The numbers are right.

But: one forward pass took *some* amount of time, and nobody measured it.

At 48 kHz, 2048 samples is **42.7 ms** — per callback, not on average.

<!-- .slide: data-state="no-footer" -->

Note:
    - Straight into step 5: mean vs. worst case.
