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

<div class="tag">Verify first</div>

## Both engines are correct<br><span class="then">Are they safe?</span>

```bash
./build/bin/step05_solution verify           # Both engines produce the reference output 

  libtorch                 max abs diff 2.21e-06   OK
  onnx                     max abs diff 5.81e-07   OK
```

Same model, same output as the Python reference.

---

<div class="tag">RealtimeSanitizer</div>

## Wrapping our buffer allocates<br><span class="then">The model hasn't run yet.</span>

<!-- `from_blob` copies nothing, but it still allocates the tensor object around our buffer. -->

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

No copy, but a new tensor object: that's the `malloc`.

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
    - Both engines were verified against the reference first: 2.21e-06 and 5.81e-07. They are correct, and they are unsafe.

---

<div class="tag">Allocation</div>

## Not once<br><span class="then">Every callback.</span>

| allocations + frees, ten callbacks | per block | first block |
|---|---|---|
| LibTorch | 10,864 | **610,429** |
| ONNX Runtime | 2,192 | 2,193 |


Not a slip. The engines are built this way.

Note:
    - Counting mode: our own operator new, ten blocks through the callback. Works without RTSan.
    -The first LibTorch block costs fifty times a normal one, even after warm-up. TorchScript recompiles while it runs.


---

<div class="tag">Allocation</div>

## Locks<br><span class="then">On the audio thread.</span>

One block, as RTSan reports it:

| | malloc | free | mutex lock |
|---|---|---|---|
| ONNX Runtime | 98 | 51 | 10 |
| LibTorch | 4,393 | 3,103 | 180 |

A slow allocation is bad. <br>
A lock is worse: it can wait on another thread.

Note:
    - RTSAN_OPTIONS=halt_on_error=false, one block only: symbolizing every report takes ~20 s for LibTorch.
    - Different counter from the previous slide, so the numbers don't add up to the same total.


---

<div class="tag">Problem</div>

## The allocation is inside the engine<br><span class="then">Nothing on our side reaches it.</span>

<div class="attempts">
  <div class="attempt"><b>Custom allocator</b><span>other memory, same locks</span></div>
  <div class="attempt"><b>One thread</b><span>no pool, same allocations</span></div>
  <div class="attempt"><b>Preallocate</b><span>ours move, theirs stay</span></div>
</div>

Note:
    - The allocation happens inside the engine, in a shared library we don't control.
    - A custom allocator changes where memory comes from, not whether a lock is taken.
    - One thread removes the engine's thread pool, not its allocations.
    - Preallocating on our side moves our allocations, not theirs.
    - Shipping this on the audio thread means hoping the allocator stays fast.

---

<div class="tag">Solution</div>

## Inference does not belong on the audio thread. <br><span class="then">So where does it go?</span>

<div class="big-question">?</div>


Note:
    - So the model runs somewhere else: on its own thread.
    - The two threads pass audio without locking, without allocating, and without waiting.
    - Next step: a worker thread plus lock-free hand-over, and the latency that comes with it.
