<h1>Step 5<br>Real-time safety</h1>

Even if it were fast enough, would we be safe?

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

Note:
- The next question is, are we even real-time safe?
- We will have a look at what actually happens on the audio thread when we call our engine
- No code this step: a demo

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
- Go through this quickly
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
    // … host block into m_input; once a model block is there, pop it, then:
    m_engine.process(m_model_block.data(), m_model.m_input_size,
                     m_state.data(), m_model.m_state_size);
    // … the result into m_output, the host block out of it
}
```

`-fsanitize=realtime` checks every call underneath at runtime and reports the first break.<br>
<div class="cards-note corner-note">Needs clang ≥ 20. On macOS: Homebrew LLVM, not Apple Clang. </div>

Note:
- To find out we will use a RealtimeSanitizer
- Build our main.cpp with RealtimeSanitizer on: checks every call underneath at runtime, also in libraries
- Since Clang 20 there's a tool for this
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
- With the sanitizer it stops in the fourth callback, the first one that runs the model, before the model has run
- If we read the stack we see that ONNX Runtime's `Run` calls a `malloc` on its way into the model.
- Even with one thread and our own buffers and tensors preallocated

---

<div class="tag">Allocation · Locks</div>

## Not once<br><span class="then">Every forward pass.</span>

```bash
RTSAN_OPTIONS=halt_on_error=false:suppress_equal_stacks=false \
  ./build/rtsan/bin/step05_solution
```

| | per forward pass | total |
|---|---|---|
| `malloc` / `free` | ~1130 each | 9019 / 9014 |
| `pthread_mutex_lock` | ~145 | 1164 |

Not a slip. The engine is built this way. 
Note:
- We can also let RTSan report every violation.
- 8 forward passes: about 1,130 mallocs and frees in every one, mostly small bookkeeping objects
- And about 145 mutex locks per forward pass
- Not a mistake: the engine is built for throughput, not for real-time

---

<div class="tag">Solution</div>

## Inference does not belong on the audio thread. <br><span class="then">So where does it go?</span>

<div class="big-question">?</div>


Note:
- Our engine is inherently not realtime safe. 
- the obvious solution: the model has to leave the audio thread
- Where does the model go?
