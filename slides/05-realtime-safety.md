<h1>Step 5<br>Real-time safety</h1>

Even if it were fast enough, would we be safe?

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

Note:
- In this step we will check if we are realtime safe.
- No code this step: a demo
- We will have a look at what actually happens on the audio thread when we call our engine?

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
- You probably know these rules
- Never allocate: the allocator might lock or need a page from the OS
- Never lock: whoever holds it might be paused by the scheduler
- No OS calls: no files, no logging
- No loops without a bound: we can't know how long they take in the worst case
- On average we're not necessarily slow, but **unbounded**
- As we learned in the last step: the average doesn't matter, the worst case does: one stall is an audible dropout

---

<div class="tag">RealtimeSanitizer</div>

## Sets the promise in the type system <span class="then">The sanitizer catches what breaks it.</span>

```cpp
// Step 3's processor, shortened: the callback makes the promise
void process_block(float* samples, size_t num_samples) [[clang::nonblocking]] {
    m_input.push(samples, num_samples);
    if (m_input.available() >= k_model.m_input_size) {
        m_input.pop(m_model_block.data(), k_model.m_input_size);
        m_engine.process(m_model_block.data(), k_model.m_input_size, m_state);
        m_output.push(m_model_block.data(), k_model.m_input_size);
    }
}
```

`-fsanitize=realtime` checks every call underneath at runtime and reports the first break.<br>
<div class="cards-note corner-note">Needs clang ≥ 20. On macOS: Homebrew LLVM, not Apple Clang. </div>

Note:
- Since Clang 20 there's a tool for this
- `[[clang::nonblocking]]`: a promise in the type system, here on our `process_block` from step 3
- Build with RealtimeSanitizer: checks every call underneath at runtime, also in libraries
- Reports the first thing that breaks the promise
<!-- - On a Mac: LLVM from Homebrew, not Apple's Clang -->

---

<div class="tag">RealtimeSanitizer</div>

## Running the model allocates<br><span class="then">Before the forward pass starts.</span>

```text
ERROR: RealtimeSanitizer: unsafe-library-call
Intercepted call to real-time unsafe function `malloc`
    operator new(unsigned long)
    OrtApis::Run(...)              ← on the way into the model
    OnnxEngine::run(...)
    OnnxEngine::process(...)
    ProcessorExample::process_block(...)   ← the fourth callback
```

One thread. Our buffers and tensors preallocated. **Still allocates.**

Note:
- Run `step05_solution`, without and with the sanitizer, live if possible
- Correct: matches the Python reference
- With the sanitizer it stops in the fourth callback, the first one that runs the model, before the model has run
- If we read the stack from the bottom up: our callback calls `process`, `process` calls ONNX Runtime's `Run`, and `Run` calls `malloc` on its way into the model.
- Even with one thread and our own buffers and tensors preallocated
- Even a small `malloc` can take too long on the audio thread

---

<div class="tag">Allocation</div>

## Not once<br><span class="then">Every forward pass.</span>

```bash
RTSAN_OPTIONS=halt_on_error=false:suppress_equal_stacks=false ./build-rtsan/bin/step05_solution
```

| | per forward pass | total |
|---|---|---|
| `malloc` | ~1130 | 9019 |
| `free` | ~1130 | 9014 |


Not a slip. The engine is built this way.

Note:
- If we build it again with `halt_on_error=false`: RTSan reports every violation instead of stopping at the first
- `suppress_equal_stacks=false`: also when the call stack is one it has seen before
- Over the whole test signal: 8 forward passes, 9,019 mallocs and almost as many (matching) frees
- About 1,130 of each in every forward pass
- These are mostly small bookkeeping objects, not audio data
- Not a small mistake: built for throughput, not for real-time

---

<div class="tag">Locks</div>

## Locks<br><span class="then">On the audio thread.</span>

The same run, as RTSan reports it:

| | per forward pass | total |
|---|---|---|
| `pthread_mutex_lock` | ~145 | 1164 |

An allocation is bad. <br>
A lock is worse! 

Note:
- On the same run
- Not only allocations: the same run, as the sanitizer reports it
- Mutex locks: about 145 in every forward pass, 1,164 in total
- Allocations were already bad, but
- A lock is worse: our thread can wait for a completely different thread

---

<div class="tag">Problem</div>

## The allocation is inside the engine<br><span class="then">Nothing on our side reaches it.</span>

<div class="attempts">
  <div class="attempt"><b>Custom allocator</b><span>other memory, same locks</span></div>
  <div class="attempt"><b>One thread</b><span>no pool, same allocations</span></div>
  <div class="attempt"><b>Preallocate</b><span>ours move, theirs stay</span></div>
</div>

Note:
- Can we fix it? Not really
- Allocations are inside the engine, a library we don't control
- Custom allocator would only change where the memory comes from, not whether a lock is taken
- Running the engine with one thread removes its thread pool, but not its allocations.
- Preallocating: moves our allocations, not the ones in onnx runtime
- On the audio thread: we're hoping the allocator stays fast. Hoping is not a strategy

---

<div class="tag">Solution</div>

## Inference does not belong on the audio thread. <br><span class="then">So where does it go?</span>

<div class="big-question">?</div>


Note:
- Can't make the engine safe: 
- the obvious solution: the model has to leave the audio thread
- Where does the model go?
