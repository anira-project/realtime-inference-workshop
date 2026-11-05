<h1>Step 6<br>Inference on a<br>worker thread</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- Step 5: the engine can't run on the audio thread
- So it gets its own thread: the worker thread

**Script:**

"In step 5 we found out that the engine can't run on the audio thread. The solution was to move the inference to a separate thread. In this step we'll give it its own thread, the worker thread."

---

<div class="tag">Motivation</div>

## The audio thread moves samples<br><span class="then">The worker runs the model.</span>

<div class="lanes">
  <div class="lane-label">audio thread</div>
  <div class="lane audio"></div>
  <div class="lane-label"></div>
  <div class="lane-gap">
    <div class="lane-queue to"><span>m_input</span><i></i></div>
    <div class="lane-queue from"><span>m_output</span><i></i></div>
  </div>
  <div class="lane-label">worker thread</div>
  <div class="lane worker"><div class="lane-run">model</div></div>
</div>

<div class="statement-points two fragment">
  <div><strong>Real-time capable</strong><br>A call takes longer than a callback; the worker gets its own time (step 4)</div>
  <div><strong>Real-time safe</strong><br>Allocations and locks leave the audio thread (step 5)</div>
</div>

Note:
- Two threads
- Audio thread keeps its pace, only moves samples
- Two ring buffers as in step 3, now thread-safe, between two threads
- Audio thread pushes into `m_input`, the worker takes 2048 out once they are there
- Worker runs the model, pushes the result into `m_output`, the audio thread pops it
- Worker may allocate. No per-callback deadline, but a budget: your rounded max inference time from step 4
- The audio thread never waits for it
- Click: this solves two problems
- Real-time safe, step 5: the engine's allocations and locks leave the audio thread
- Real-time capable, step 4: even a typical call of 13 ms is longer than a callback; the worker isn't inside any callback, it gets its own time

**Script:**

"Here's the idea. We now have two threads. The audio thread keeps its pace: in every callback it only moves samples around, nothing else. It pushes the host's samples into the input ring buffer, `m_input` as in step 3, but now a thread-safe one. Once there are 2048 samples in it, the worker takes them out, runs the model and pushes the result into `m_output`, where the audio thread picks it up.
The worker is allowed to allocate. It doesn't have a per-callback deadline. It does have a budget: the rounded max inference time you measured in step 4. The audio thread never waits for it.

*Click: the two problems.*

So with the worker thread we solve two problems at once. First, real-time safety from step 5: the engine's allocations and locks are no longer on the audio thread. And second, real-time capability from step 4: even a typical call of 13 milliseconds is longer than a callback. Now the forward pass no longer runs inside any callback, it gets its own time."

---

<div class="tag">Goal</div>

## The goal<br><span class="then">Split step 3's processor over two threads.</span>


<div class="statement-points">
  <div><small>1</small>Size the ring buffers</div>
  <div><small>2</small>Hand samples over on the audio thread</div>
  <div><small>3</small>Run the model on the worker thread and return the results</div>
</div>

Note:
- Goal: step 3's processor on two threads, with a fixed latency
- `prepare`: size `m_input` and `m_output`; the prefill and the worker thread itself are given
- Audio thread: samples into `m_input`, samples out of `m_output`, never waiting
- Worker: take a block out of `m_input`, run the model, push it into `m_output`

**Script:**

"So our goal is to split our processor from step 3 over two threads, with a latency that stays fixed. First, in `prepare`, we give the ring buffers their sizes. Starting the thread is given. Then we write the audio thread's side: samples into `m_input`, samples out of `m_output`, and it never waits. And then the worker's side: take a block out of `m_input`, run the model, push it into `m_output`."

---

<div class="tag">Given</div>

## What we get <span class="then">3 new helpers, 1 file to edit.</span>



<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">common/assets/</div>
    {{FILE:model_forward.onnx}}
    {{FILE:test_signal.h}}
    {{FILE:target_signal.h}}
  </div>
  <div class="given-column many">
    <div class="given-label">Helpers</div>
    <div class="given-folder">common/helpers/ · CMake</div>
    {{FILE:onnx_engine.h}}
    {{FILE:host.h|new}}
    {{FILE:threadsafe_ring_buffer.h|new}}
    {{FILE:worker_thread.h|new}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">06_threading/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
- Still the model, signals and engine
- New: `ThreadSafeRingBuffer`, step 3's ring buffer safe between two threads, on moodycamel, fetched by CMake
- New: `host.h`, how long one block lasts: step 3's host loop sleeps that long after every callback, like an audio device
- New: `WorkerThread`, calls one function over and over on its own thread, `start` and `stop`

**Script:**

"We still have the model, the signals and the engine from the steps before. New is the `ThreadSafeRingBuffer`: it replaces the ring buffer from step 3, with the same functions, but safe between two threads. Underneath is a lock-free buffer from moodycamel, which CMake fetches for us. Our host loop from step 3 gets one line more: after every callback it sleeps for as long as one block of audio lasts, 11.6 milliseconds. With two threads, this only works if the audio thread is called at the audio rate, and the sleep simulates the audio device. `host.h` gives us that duration. The `WorkerThread` takes care of the thread: it calls one function over and over, from `start` until `stop`."

---

<div class="tag">Helpers: <code>threadsafe_ring_buffer.h</code></div>

## One writer, one reader<br><span class="then">No locks.</span>

```cpp
ThreadSafeRingBuffer m_input{capacity};   // One writer, one reader
// Underneath: moodycamel::BlockingReaderWriterCircularBuffer<float>, try_ only

m_input.try_push(sample);                 // audio thread: false if full
m_input.available();                      // worker: a whole block there?
m_input.pop(block, 2048);
m_output.try_pop(sample);                 // audio thread: false if empty

WorkerThread m_worker;
m_worker.start([this] { process_model(); });  // over and over, until stop()
```

<div class="queue-demo" style="margin-top: var(--space-5)">
  <div class="q">
    <div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div>
  </div>
  <i class="q-chip" style="--n:0"></i>
  <i class="q-chip" style="--n:1"></i>
  <i class="q-chip" style="--n:2"></i>
  <i class="q-chip" style="--n:3"></i>
  <i class="q-chip bounce"></i>
  <div class="q-fail"><code>try_push()</code> fails → a zero goes out instead</div>
</div>

Note:
- One thread writing, one reading: the simplest lock-free case
- Step 3's `push`, `pop`, `available` for the worker's blocks
- `try_push` and `try_pop` for the audio thread: one sample, and they say whether it worked
- Underneath: moodycamel's circular buffer, only its `try_` functions: never wait, never allocate, never throw
- `available`: the worker checks for a whole block, then pops it
- `WorkerThread`: the thread is given, `process_model` runs in it over and over
- Full or empty: no waiting. Waiting would be a missed deadline for the whole host

**Script:**

"Let's look at the helpers. Each ring buffer has exactly one thread writing and one thread reading, which is the simplest lock-free case there is. It has step 3's functions, `push`, `pop` and `available`, which the worker uses for whole blocks. And it has `try_push` and `try_pop` for the audio thread: they move one sample and tell us whether it worked. Underneath is a circular buffer from moodycamel, and we only use its try functions, which never wait, never allocate and never throw. The worker uses `available` to see whether a whole block is there, and then pops it. And the thread itself is given: the `WorkerThread` calls our `process_model` over and over, from `start` until `stop`. Nothing here waits: on the audio thread, waiting would be a missed deadline for the whole host."
  

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="10"></div>

Fill in the TODOs in `steps/06_threading/exercise/main.cpp`

```bash
cmake --build --preset release --target step06_exercise
./build/bin/step06_exercise
```

<div class="run">
  <div class="run-step" data-seconds="2.4">
    <div class="run-title"><span>1</span>Give the ring buffer a sizes
  </div>
    <div class="run-visual run-setup">
      <div class="q small appear"><div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div></div>
      <div class="q small appear"><div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div></div>
      <div class="setup-thread">worker</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>2</span>The audio thread</div>
    <div class="run-visual run-audio">
      <div class="q small">
        <div class="q-slot"><b></b></div><div class="q-slot"><b></b></div><div class="q-slot"><b></b></div><div class="q-slot"></div>
      </div>
      <div class="run-note">no engine</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>3</span>The worker thread</div>
    <div class="run-visual run-worker">
      <div class="q small one"><div class="q-slot"><b></b></div></div>
      <div class="mini-model">model</div>
      <div class="q small one out"><div class="q-slot"><b></b></div></div>
      <div class="run-ok">✓ the model's stream, the prefill late</div>
    </div>
  </div>
</div>

Note:
- First: your number from step 4 into `k_max_inference_ms` in `steps/common/inference_time.h`, if not done yet
- Three TODOs: the ring buffer sizes in `prepare`, the audio thread, the worker thread
- `main.cpp` in steps/06_threading/exercise, build commands on the slide
- The biggest task so far, one line at a time: 10 minutes
- When it works: the host hears the model's stream, exactly the prefill late
- Solution is in the `solution` folder

**Script:**

"Now it's your turn. First, if you haven't yet, put your number from step 4 into `k_max_inference_ms` in `steps/common/inference_time.h`. Then there are three TODOs: the ring buffer sizes in `prepare`, the audio thread, and the worker thread. You'll find the `main.cpp` in `steps/06_threading/exercise`, and the build commands are on the slide. This is the biggest task so far, but it's one line at a time, so you get 10 minutes. When it works, the host hears the model's stream, exactly the prefill late. And as always, the solution is in the `solution` folder."

*Timebox.*

---

<div class="tag">Solution</div>

## TODO 1: The ring buffer sizes

```cpp
// given: how much silence m_output starts with
m_prefill = (k_model.m_input_size - max_block_size)
    + k_max_inference_samples_rounded;

const size_t input_capacity = k_model.m_input_size;  // <- fill out
const size_t output_capacity =
    k_max_inference_samples_rounded + k_model.m_input_size;  // <- fill out

// given: the silence goes in, and the worker thread starts
m_output.push(silence.data(), m_prefill);
m_worker.start([this] { process_model(); });
```

Note:
- `m_input`: one model block, as in step 3
- `m_output`: the worker's time plus one model block
- Given: `k_max_inference_samples_rounded`, your worst case from step 4, in samples, rounded up to whole callbacks: the worker's time
- Given: the prefill, the silence `m_output` starts with, so the worker has time. How much that is: step 7
- Then the worker starts, given
- All in `prepare`: the ring buffers and the thread allocate here, once, so the callback never has to

**Script:**

"Let's look at the solutions together. The input ring buffer keeps one model block, like in step 3. The output ring buffer needs room for the worker's time plus one model block. The worker's time is given: it's your worst case from step 4, in samples, rounded up to whole callbacks. The output also starts with some silence, the prefill, so that the worker has time and the host always finds something to take. That's given too, and in the next step we'll look at how much it has to be. And then the worker starts. It's important that all of this happens in `prepare`: the ring buffers allocate their memory here, once, so the callback never has to."

---

<div class="tag">Solution</div>

## TODO 2: The audio thread

<!-- .slide: class="tight-code" -->

<div class="output-full large">

```cpp
for (size_t i = 0; i < num_samples; ++i) {
    const bool pushed = m_input.try_push(samples[i]);        // <- fill out
    if (!pushed) { m_unpushed += num_samples - i; break; }  // given: count the rest
}
const size_t zeros = std::min(m_unpushed, num_samples);     // given: zeros for those
std::fill_n(samples, zeros, 0.0f);
m_unpushed -= zeros;

while (m_debt > 0) {                                        // pay the debt first
    if (m_output.try_pop(late)) { --m_debt; } else { break; }  // given
}

for (size_t i = zeros; i < num_samples; ++i) {
    const bool popped = m_debt == 0 && m_output.try_pop(samples[i]);  // <- fill out
    if (!popped) {                                    // late: the rest is zeros, all owed
        std::fill_n(samples + i, num_samples - i, 0.0f);
        m_debt += num_samples - i;                                     // given
        break;
    }
}
```

</div>

Note:
- Step 3's `process_block`, without the engine, one sample at a time
- To fill in: only `try_push` and `try_pop`. The zeros and the debt around them are given
- Push: if the input is full, the worker is a whole block behind. Count the rest, don't wait
- Those never reach the model: the host gets as many zeros, and nothing comes out of `m_output` for them
- Pop: if a sample isn't there yet, or something is still owed, the worker is late: the rest of the callback goes out as zeros, all owed: the debt
- Not just that one sample: the worker may be pushing right now, and the next one would land one place too early
- When the late samples arrive, their place already went out as zeros: take them out, throw them away
- So the stream stays exactly the prefill late, a dropout doesn't shift it
- Not in here: no `new`, no mutex, no engine, no waiting

**Script:**

"The audio thread is step 3's process_block, without the engine, and one sample at a time. You only had to fill in the `try_push` and the `try_pop`; the rest is given, but let's look at why it's there. We push the host's samples into the input ring buffer. If one doesn't fit, the worker is a whole block behind. We don't wait: we count the rest and stop. Those samples never reach the model, so the host gets as many zeros instead, and nothing is taken out of the output for them.
Then we hand the host its samples from the output ring buffer. If a sample isn't there yet, or we still owe something, the worker is late: the rest of this callback goes out as zeros, and we owe all of those samples: that's the debt. Why the whole rest and not just that one sample? The worker may be pushing at this very moment, and the next sample we'd get would land one place too early. When the late samples finally arrive, their place already went out as zeros, so we take them out and throw them away. That way a dropout is a dropout, and it doesn't make the rest of the stream later.
Notice what's not in here: no `new`, no mutex, no engine, and no waiting."

---

<div class="tag">Solution</div>

## TODO 3: The worker thread

```cpp
void process_model() {  // called over and over by m_worker
    const bool has_block = m_input.available() >= k_model.m_input_size;  // <- fill out

    if (has_block) {
        m_input.pop(m_model_block.data(), k_model.m_input_size);         // <- fill out
        m_engine.process(m_model_block.data(), k_model.m_input_size, m_state);
        m_output.push(m_model_block.data(), k_model.m_input_size);       // <- fill out
    }
}
```

Note:
- The other half of step 3's `process_block`, the same lines
- `WorkerThread` calls it over and over: no block yet, it checks again right away, no sleep
- That keeps one core busy; part 2 does it better
- A whole block: take it out, run the model, push it into `m_output`
- The engine may allocate and lock here: nobody waits on this thread

**Script:**

"The worker side is the other half of step 3's process_block, with the same lines. The `WorkerThread` calls it over and over. If there's no whole block in the input ring buffer yet, it simply checks again right away, without sleeping, because the result is needed soon. That keeps one core busy, and in part 2 we'll see how to do it better. If there is a block, we take it out, run the model and push the result into the output ring buffer. And here the engine may allocate and lock as much as it wants: nobody is waiting on this thread."

---

<div class="tag">Done</div>

## The audio thread is real-time safe<br><span class="then">The model runs beside it.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
- Same stream as step 2, the engine is off the audio thread
- Audio thread is real-time safe, the model runs beside it
- If possible, run it next to step 5 live
- The host hears it exactly the prefill late
- But the model itself answers late too. One number for the host, and a dry signal: next step

**Script:**

"The result: the same stream as in step 2, and the engine is off the audio thread. The audio thread is now real-time safe, and the model runs beside it."
*If possible, run it next to step 5 live.*
"And the host hears it exactly the prefill late. But the model itself answers late too. So what's the one number we tell the host, and how do we line up a dry signal with it? That's our next step: latency."
