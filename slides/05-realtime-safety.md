<h1>Step 5<br>Real-time safety</h1>

Even if it were fast enough, it wouldn't be safe

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

Note:
- Step 4: doesn't fit one small callback, and even if it did, it wouldn't be safe
- No code this step: a demo
- What actually happens on the audio thread when we call our engine?

**Script:**

"In step 4 we saw that the forward pass doesn't fit into one small callback. But even if it did, it wouldn't be safe. In this step we won't write code, we'll run a demo and look at what actually happens on the audio thread when we call our engine."

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
- No loops without a bound
- Not necessarily slow, but **unbounded**
- The average doesn't matter, the worst case does: one stall is an audible dropout

**Script:**

"You probably know these rules already. Inside the audio callback we never allocate memory, because the allocator might lock or need a new page from the operating system. We never lock, because whoever holds the lock might be paused by the scheduler. We don't call the OS, so no files, no logging. And we don't run loops without a bound.
The important point is: these things aren't necessarily slow. Most of the time they're fast. The problem is that they're **unbounded**: we can't know how long they take in the worst case. And as we saw in step 4, the average doesn't matter. The worst case does. One stall is enough for an audible dropout."

---

<div class="tag">RealtimeSanitizer</div>

## Sets the promise in the type system <span class="then">The sanitizer catches what breaks it.</span>

```cpp
// Step 3's processor: the callback makes the promise
void process_block(float* samples, size_t num_samples) [[clang::nonblocking]] {
    m_input.push(samples, num_samples);
    if (m_input.available() >= k_model.m_input_size) {
        m_input.pop(m_model_block.data(), k_model.m_input_size);
        m_engine.process(m_model_block.data(), k_model.m_input_size, m_state);
        m_output.push(m_model_block.data(), k_model.m_input_size);
    }
    …
}
```

`-fsanitize=realtime` checks every call underneath it, at runtime, and reports the first thing that breaks the promise.

Needs clang ≥ 20. On macOS: Homebrew LLVM, not Apple Clang.

Note:
- Since Clang 20 there's a tool for this
- `[[clang::nonblocking]]`: a promise in the type system, here on step 3's `process_block`
- Build with RealtimeSanitizer: checks every call underneath at runtime, also in libraries
- Reports the first thing that breaks the promise
- On a Mac: LLVM from Homebrew, not Apple's Clang

**Script:**

"So how do we find out whether our code breaks these rules? Since Clang 20 there's a tool for it. We take our processor from step 3 and mark its callback with `[[clang::nonblocking]]`, which is a promise in the type system: nothing in here blocks. Then we build with the RealtimeSanitizer. At runtime it checks every call underneath that function, also inside libraries, and reports the first thing that breaks the promise.
If you want to try this on a Mac, you'll need the LLVM from Homebrew, Apple's Clang doesn't have it yet."

---

<div class="tag">RealtimeSanitizer</div>

## Running the model allocates<br><span class="then">Before the forward pass starts.</span>

```bash
./build/bin/step05_solution                  # Without RTSan: runs through, matches the reference
  OK: max abs diff 3.87e-07, within 0.0001 of the reference.
```

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
- Stack from the bottom: `process_block` → `process` → `Run` → `malloc`
- Even warmed up in `prepare()`, with one thread and our own buffers and tensors preallocated
- Even a small `malloc` can take too long on the audio thread

**Script:**

*Run `step05_solution`, without and with the sanitizer, live if possible.*
"First, is it still correct? Yes, without the sanitizer it runs through and gives us the same output as the Python reference. But is it safe? With the sanitizer it stops in the fourth callback, the first one that runs the model, before the model has even run. If we read the stack from the bottom up: our callback calls `process`, `process` calls ONNX Runtime's `Run`, and `Run` calls `malloc` on its way into the model. And this is warmed up, on one thread, with our own buffers and tensors preallocated. It's only a small allocation, but even a small `malloc` can take too long on the audio thread."

---

<div class="tag">Allocation</div>

## Not once<br><span class="then">Every callback.</span>

```bash
RTSAN_OPTIONS=halt_on_error=false:suppress_equal_stacks=false ./build-rtsan/bin/step05_solution
```

| the whole test signal, 8 forward passes | total | per forward pass |
|---|---|---|
| `malloc` | 8620 | 1078 |
| `free` | 8619 | 1077 |


Not a slip. The engine is built this way.

Note:
- `halt_on_error=false`: RTSan reports every violation instead of stopping at the first
- `suppress_equal_stacks=false`: also when the call stack is one it has seen before
- Over the whole test signal: 8 forward passes, 8,620 mallocs and as many frees
- About 1,080 of each in every forward pass
- Not a small mistake: built for throughput, not for real-time

**Script:**

"And it doesn't happen just once. If we tell the sanitizer not to stop at the first violation, it reports all of them. Over the whole test signal, eight forward passes, that's 8,620 mallocs and as many frees: about 1,080 of each in every single forward pass.
So this isn't a small mistake we can fix. The engine is built this way: it's made for throughput, not for real-time."


---

<div class="tag">Locks</div>

## Locks<br><span class="then">On the audio thread.</span>

The same run, as RTSan reports it:

| | total | per forward pass |
|---|---|---|
| `pthread_mutex_lock` | 608 | 76 |

A slow allocation is bad. <br>
A lock is worse: it can wait on another thread.

Note:
- Not only allocations: the same run, as the sanitizer reports it
- Mutex locks: 608, 76 in every forward pass
- A lock is worse: our thread can wait for a completely different thread

**Script:**

"And it's not only allocations. In the same run the sanitizer also reports mutex locks: 608 of them, 76 in every forward pass. A slow allocation is bad. But a lock is worse: it can make our audio thread wait for a completely different thread."


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
- Custom allocator: where the memory comes from, not whether a lock is taken
- One thread: no thread pool, allocations stay
- Preallocating: moves our allocations, not theirs
- On the audio thread we're hoping the allocator stays fast. Hoping is not a strategy

**Script:**

"So can we fix this? Not really. The allocations happen inside the engine, in a library we don't control. A custom allocator only changes where the memory comes from, not whether a lock is taken. Running the engine with one thread removes its thread pool, but not its allocations. And preallocating everything on our side only moves *our* allocations, not theirs.
So if we ship this on the audio thread, we're basically hoping the allocator stays fast. And hoping is not a strategy."

---

<div class="tag">Solution</div>

## Inference does not belong on the audio thread. <br><span class="then">So where does it go?</span>

<div class="big-question">?</div>


Note:
- Can't make the engine safe: the model has to leave the audio thread
- The audio thread keeps moving samples
- Where does the model go? How does the audio get there and back, within the rules?
- Next step

**Script:**

"So if we can't make the engine safe, there's only one option left: the model has to leave the audio thread. The audio thread keeps doing what it's good at, moving samples. But running the model is not its job anymore.
So where does it go? And how do we get the audio there and back, without breaking the rules we just talked about? That's what the next step is about."
