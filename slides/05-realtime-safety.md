<h1>Step 5<br>Real-time safety</h1>

Fast enough is not the same as safe

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

Note:
    - Demonstration, no exercise. Run it live if the Homebrew clang is there; the output is the slide.

---
<div class="tag">Promise</div>

## The audio thread has a deadline<br><span class="then">Nothing in it may take unbounded time.</span>

Inside the callback, never:

- **allocate**: the allocator may lock or page-fault
- **lock**: the holder may be descheduled
- **call the OS**: no files, no logging, no syscalls
- **loop without a bound**

The average doesn't matter. **The worst case does.**

Note:
    - Not "slow", unbounded. The 99.9th percentile is where it bites: one stall is an audible dropout.

---

<div class="tag">RealtimeSanitizer</div>

## Sets the promise in the type system <span class="then">The sanitizer catches what breaks it.</span>

```cpp
void audio_callback(Engine& engine, float* samples, size_t n)
    [[clang::nonblocking]]
{
    engine.process(samples, n);
}
```

`-fsanitize=realtime` checks every call underneath it, at runtime, and reports the first thing that breaks the promise.

Needs clang ≥ 20. On macOS: Homebrew LLVM, not Apple Clang.

---

<div class="tag">RealtimeSanitizer</div>

## Wrapping our buffer allocates<br><span class="then">The model hasn't run yet.</span>


```
ERROR: RealtimeSanitizer: unsafe-library-call
Intercepted call to real-time unsafe function `malloc`
    operator new(unsigned long)
    c10::intrusive_ptr<c10::StorageImpl, ...>::make<...>
    at::TensorMaker::make_tensor()
    torch::from_blob(...)          ← only wraps our buffer
    LibTorchEngine::process(...)
    audio_callback(...)
```


---

<div class="tag">ONNX Runtime</div>

## ONNX Runtime <br><span class="then">Same story, other library.</span>

```
ERROR: RealtimeSanitizer: unsafe-library-call
Intercepted call to real-time unsafe function `malloc`
    OrtApis::CreateTensorWithDataAsOrtValue(...)   ← also only wraps
    OnnxEngine::process(...)
    audio_callback(...)
```

One thread. Fixed arena. Our buffers preallocated.<br>
**Still allocates.**


Note:
    - Both engines were verified against the reference first: 4.9e-07 and 3.9e-07. They are correct, and they are unsafe.

---

<div class="tag">Not once — all the time</div>

## Mutexes<br><span class="then">On the audio thread.</span>

| allocations + frees, ten callbacks | per block | first block |
|---|---|---|
| LibTorch | 10,864 | **610,429** |
| ONNX Runtime | 2,192 | 2,193 |

```
ONNX Runtime     99 × malloc    50 × free    11 × mutex lock/unlock
LibTorch       4062 × malloc  2413 × free   180 × mutex lock/unlock
```

Note:
    - The code block is what RTSan reports for one block.
    - Block 0 at 610k: TorchScript recompiles while it runs, after the warm-up call. That is the 56 ms outlier from step 3.

---

<div class="tag">No flag for it</div>

## The allocation is inside the engine.

- The allocation happens **inside** the engine, in a shared library
- A custom allocator changes where memory comes from, not whether a lock is taken
- One thread removes the pool, not the allocations
- Preallocating on your side moves your allocations, not theirs

Shipping this on the audio thread means hoping the allocator stays fast.

---

<div class="tag">What that leaves</div>

## Inference does not belong<br><span class="then">on the audio thread.</span>

<div class="statement-points">
  <div>So it runs somewhere else</div>
  <div>The two threads pass audio without locking</div>
  <div>…without allocating, and without waiting</div>
</div>


Note:
    - Next step: a worker thread plus lock-free hand-over, and the latency that comes with it.
