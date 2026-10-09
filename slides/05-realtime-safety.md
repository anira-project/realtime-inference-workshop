<h1>Step 5<br>Real-time safety</h1>

Fast enough is not the same as safe

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

Note:
    - Demonstration, no exercise. Run it live if the Homebrew clang is there; the output is the slide.

---

<div class="tag">The promise</div>

## Not slow. Unbounded.

The audio thread has a deadline and no second chance. So, inside a callback:

- no allocation — the allocator can take a lock and a page fault
- no lock — whoever holds it may be descheduled
- no syscall, no file, no logging
- no unbounded loop

Not "slow". **Unbounded.** The 99.9th percentile is where it bites.

---

<div class="tag">RealtimeSanitizer</div>

## Say it in the type system.

```cpp
void audio_callback(Engine& engine, float* samples, size_t n)
    [[clang::nonblocking]]
{
    engine.process(samples, n);
}
```

`-fsanitize=realtime` then checks every call underneath it, at runtime, and
reports the first thing that breaks the promise.

Needs clang ≥ 20. On macOS: Homebrew LLVM, not Apple Clang.

---

<div class="tag">LibTorch</div>

## It allocates before the model even runs.

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

The model has not run yet.

---

<div class="tag">ONNX Runtime</div>

## Same story, other library.

```
ERROR: RealtimeSanitizer: unsafe-library-call
Intercepted call to real-time unsafe function `malloc`
    OrtApis::CreateTensorWithDataAsOrtValue(...)   ← also only wraps
    OnnxEngine::process(...)
    audio_callback(...)
```

One thread, no arena growth, our own side
preallocated — and it still allocates on the way in.

Note:
    - Both engines were verified against the reference first: 2.2e-06 and 5.8e-07. They are correct, and they are unsafe.

---

<div class="tag">Not once — all the time</div>

## Mutexes.<br><span class="then">On the audio thread.</span>

| allocations + frees, ten callbacks | per block | first block |
|---|---|---|
| LibTorch | 10,960 | **591,393** |
| ONNX Runtime | 1,932 | 2,000-ish |

```
ONNX Runtime     98 × malloc    51 × free    10 × mutex lock/unlock
LibTorch       4393 × malloc  3103 × free   180 × mutex lock/unlock
```

Note:
    - The code block is what RTSan reports for one block.
    - Block 0 at 591k: TorchScript recompiles while it runs, after the warm-up call. That is the 84 ms outlier from step 3.

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
