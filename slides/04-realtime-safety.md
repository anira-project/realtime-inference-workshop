<h1>Step 4<br>Real-time safety</h1>

Even if it were fast enough, it wouldn't be safe

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

Note:
    - Demonstration, no exercise. Run it live if the Homebrew clang is there; the output is the slide.

**Script:**

"In step 3 we saw that the forward pass doesn't fit into one small callback. But even if it did, it wouldn't be safe. In this step we won't write code, we'll run a demo and look at what actually happens on the audio thread when we call our engine."

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

**Script:**

"You probably know these rules already. Inside the audio callback we never allocate memory, because the allocator might lock or need a new page from the operating system. We never lock, because whoever holds the lock might be paused by the scheduler. We don't call the OS, so no files, no logging. And we don't run loops without a bound.
The important point is: these things aren't necessarily slow. Most of the time they're fast. The problem is that they're **unbounded**: we can't know how long they take in the worst case. And as we saw in step 3, the average doesn't matter. The worst case does. One stall is enough for an audible dropout."

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

Note:
**Script:**

"So how do we find out whether our code breaks these rules? Since Clang 20 there's a tool for it. We mark our callback with `[[clang::nonblocking]]`, which is a promise in the type system: nothing in here blocks. Then we build with the RealtimeSanitizer. At runtime it checks every call underneath that function, also inside libraries, and reports the first thing that breaks the promise.
If you want to try this on a Mac, you'll need the LLVM from Homebrew, Apple's Clang doesn't have it yet."

---

<div class="tag">Verify first</div>

## Both engines are correct<br><span class="then">Are they safe?</span>

```bash
./build/bin/step04_solution verify           # Both engines produce the reference output 

  libtorch                 max abs diff 4.92e-07   OK
  onnx                     max abs diff 3.87e-07   OK
```

Same model, same output as the Python reference.

Note:
**Script:**

*Run `step04_solution verify` live if possible.*
"Before we check whether it's safe, let's make sure it's correct. For this demo we use two engines. One is LibTorch, which we've used so far. The other is ONNX Runtime, a much smaller engine. ONNX is an open file format for models, so we exported the same model to it, and ONNX Runtime is the engine that runs that file. We'll compare the two properly in step 7. Both give us the same output as the Python reference, the difference is tiny. So both are correct. The question is: are they safe?"

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

Note:
**Script:**

*Run the sanitizer live.*
"If we run the LibTorch engine with the sanitizer, it stops immediately, before the model has even run. If we read the stack from the bottom up: our audio callback calls `process`, `process` calls `from_blob`, and `from_blob` calls `malloc`.
Remember step 1? `from_blob` only wraps our buffer, it doesn't copy anything. That's true. But it still creates a new tensor object around our buffer, and that object needs memory. So there is no copy, but still an allocation."

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

One thread. Our buffers preallocated.<br>
**Still allocates.**


Note:
    - Both engines were verified against the reference first: 4.92e-07 and 3.87e-07. They are correct, and they are unsafe.

**Script:**

"So maybe ONNX Runtime is better? Not really. We have the same story, but a different library. Creating the tensor that wraps our buffer allocates too. And this is with ONNX Runtime on one thread and our own buffers preallocated. It still allocates. So both engines are correct, but neither of them is real-time safe."

---

<div class="tag">Allocation</div>

## Not once<br><span class="then">Every callback.</span>

| allocations + frees, ten callbacks | per block | first block |
|---|---|---|
| LibTorch | 8,904 | **600,629** |
| ONNX Runtime | 2,192 | 2,193 |


Not a slip. The engines are built this way.

Note:
    - Counting mode: our own operator new, ten blocks through the callback. Works without RTSan.
    -The first LibTorch block costs about 67 times a normal one, even after warm-up. TorchScript recompiles while it runs.

**Script:**

"And it doesn't happen just once. Here we counted the allocations and frees over ten callbacks. LibTorch does about nine thousand per block, and in the very first block more than half a million, even though we already ran one block as a warm-up, because TorchScript is still optimising the model while it runs. ONNX Runtime is much better, but still about two thousand per block.
So this isn't a small mistake we can fix. The engines are built this way: they're made for throughput, not for real-time."


---

<div class="tag">Locks</div>

## Locks<br><span class="then">On the audio thread.</span>

One block, as RTSan reports it:

| | mutex locks |
|---|---|
| ONNX Runtime | 11 |
| LibTorch | 180 |

A slow allocation is bad. <br>
A lock is worse: it can wait on another thread.

Note:
    - RTSAN_OPTIONS=halt_on_error=false, one block only: symbolizing every report takes ~20 s for LibTorch.

**Script:**

"And it's not only allocations. Here's one single block, as the sanitizer reports it: there are also mutex locks. Eleven for ONNX Runtime, 180 for LibTorch. A slow allocation is bad. But a lock is worse: it can make our audio thread wait for a completely different thread."


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

**Script:**

"So can we fix this? Not really. The allocations happen inside the engine, in a library we don't control. A custom allocator only changes where the memory comes from, not whether a lock is taken. Running the engine with one thread removes its thread pool, but not its allocations. And preallocating everything on our side only moves *our* allocations, not theirs.
So if we ship this on the audio thread, we're basically hoping the allocator stays fast. And hoping is not a strategy."

---

<div class="tag">Solution</div>

## Inference does not belong on the audio thread. <br><span class="then">So where does it go?</span>

<div class="big-question">?</div>


Note:
    - So the model runs somewhere else: on its own thread.
    - The two threads pass audio without locking, without allocating, and without waiting.
    - Next step: a worker thread plus lock-free hand-over, and the latency that comes with it.

**Script:**

"So if we can't make the engine safe, there's only one option left: the model has to leave the audio thread. The audio thread keeps doing what it's good at, moving samples. But running the model is not its job anymore.
So where does it go? And how do we get the audio there and back, without breaking the rules we just talked about? That's what the next step is about."
