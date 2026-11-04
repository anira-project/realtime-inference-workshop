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
  <div><strong>Real-time capable</strong><br>A slow call is 3.7 callbacks; the worker gets its own time (step 4)</div>
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
- Real-time capable, step 4: a slow call of 43 ms was 3.7 callbacks; the worker isn't inside any callback, it gets its own time

**Script:**

"Here's the idea. We now have two threads. The audio thread keeps its pace: in every callback it only moves samples around, nothing else. It pushes the host's samples into the input ring buffer, `m_input` as in step 3, but now a thread-safe one. Once there are 2048 samples in it, the worker takes them out, runs the model and pushes the result into `m_output`, where the audio thread picks it up.
The worker is allowed to allocate. It doesn't have a per-callback deadline. It does have a budget: the rounded max inference time you measured in step 4. The audio thread never waits for it.

*Click: the two problems.*

So with the worker thread we solve two problems at once. First, real-time safety from step 5: the engine's allocations and locks are no longer on the audio thread. And second, real-time capability from step 4: a slow call of 43 milliseconds was almost four callbacks long. Now the forward pass no longer runs inside any callback, it gets its own time."

---

<div class="tag">Goal</div>

## The goal<br><span class="then">Create two threads.</span>


<div class="statement-points">
  <div><small>1</small>Prefill the output and start the worker</div>
  <div><small>2</small>Hand samples over on the audio thread</div>
  <div><small>3</small>Run the model on the worker thread and return the results</div>
</div>

Note:
- Goal: set up the two threads, with a fixed latency
- `prepare`: work out the prefill, size `m_output`, start the worker
- Audio thread: samples into `m_input`, samples out of `m_output`, never waiting
- Worker: take a block out of `m_input`, run the model, push it into `m_output`

**Script:**

"So our goal is to set up these two threads, with a latency that stays fixed. First, in `prepare`, we work out how much silence the output starts with, size the output ring buffer and start the worker. Then we write the audio thread's side: samples into `m_input`, samples out of `m_output`, and it never waits. And then the worker's side: take a block out of `m_input`, run the model, push it into `m_output`."

---

<div class="tag">Given</div>

## What we get <span class="then">2 new helpers, 1 file to edit.</span>



<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">models/ · common/assets/</div>
    {{FILE:model.pt}}
    {{FILE:test_signal.h}}
    {{FILE:target_signal.h}}
  </div>
  <div class="given-column many">
    <div class="given-label">Helpers</div>
    <div class="given-folder">common/helpers/ · CMake</div>
    {{FILE:libtorch_engine.h}}
    {{FILE:ring_buffer.h}}
    {{FILE:host.h|new}}
    {{FILE:threadsafe_ring_buffer.h|new}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">06_threading/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
- Still the model, signals, engine and ring buffer
- New: `ThreadSafeRingBuffer`, step 3's ring buffer safe between two threads, on moodycamel, fetched by CMake
- New: `run_host` in `host.h`, step 3's host loop in real time: waits between callbacks, 512 samples per call
- `main.cpp` counts allocations: check the audio thread doesn't allocate anymore

**Script:**

"We still have the model, the signals, the engine and the ring buffer from the steps before. New is the `ThreadSafeRingBuffer`: the ring buffer from step 3, with the same functions, but safe between two threads. Underneath is a lock-free buffer from moodycamel, which CMake fetches for us. Also new is `host.h`: the host loop from step 3, but in real time, so it waits between the callbacks like a real audio device, with 512 samples per call. And in `main.cpp` there's a counter for allocations, so we can check that the audio thread really doesn't allocate anymore."

---

<div class="tag">Helpers: <code>threadsafe_ring_buffer.h</code></div>

## One writer, one reader<br><span class="then">No locks.</span>

```cpp
ThreadSafeRingBuffer m_input{capacity};   // One writer, one reader

m_input.try_push(sample);                 // audio thread: false if full
m_input.available();                      // worker: a whole block there?
m_input.pop(block, 2048);

m_output.try_pop(sample);                 // audio thread: false if empty

// Underneath: moodycamel::BlockingReaderWriterCircularBuffer<float>, try_ only
```


<div class="queue-demo">
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
- Underneath: moodycamel's circular buffer, only its `try_` functions, which never wait and never allocate
- `try_push` and `try_pop` move one sample and say whether it worked
- `available`: the worker checks for a whole block, then pops it
- Input full: the sample can't go in, the host gets a zero for it on the way out
- A glitch, but waiting would be a missed deadline for the whole host

**Script:**

"Let's look at the ring buffer. Each buffer has exactly one thread writing and one thread reading, which is the simplest lock-free case there is. Underneath is a circular buffer from moodycamel, and we only use its try functions, which never wait and never allocate. `try_push` and `try_pop` move one sample and tell us whether it worked. The worker uses `available` to see whether a whole block is there, and then pops it. If the input ring buffer is full, a sample can't go in, and the host gets a zero for it on the way out. That's a glitch, but waiting would be a missed deadline for the whole host."
  

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
    <div class="run-title"><span>1</span>Prefill and the worker</div>
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
      <div class="run-note">allocations: 0</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>3</span>The worker thread</div>
    <div class="run-visual run-worker">
      <div class="q small one"><div class="q-slot"><b></b></div></div>
      <div class="mini-model">model</div>
      <div class="q small one out"><div class="q-slot"><b></b></div></div>
      <div class="run-ok">✓ 0 allocations on the audio thread</div>
    </div>
  </div>
</div>

Note:
- First: your number from step 4 into `k_max_inference_ms`
- Three TODOs: `prepare` (prefill + worker), the audio thread, the worker thread
- `main.cpp` in steps/06_threading/exercise, build commands on the slide
- Biggest task so far, but one line at a time: 5 minutes
- When it works: 0 allocations on the audio thread, the host hears the stream the prefill late
- Solution is in the `solution` folder

**Script:**

"Now it's your turn. First, put the number you wrote down in step 4 into `k_max_inference_ms`. Then there are three TODOs: the prefill and the worker in `prepare`, the audio thread, and the worker thread. You'll find the `main.cpp` in `steps/06_threading/exercise`, and the build commands are on the slide. This is the biggest task so far, but it's one line at a time, so you get 5 minutes. When it works, the program should report 0 allocations on the audio thread, and the host hears the model's stream exactly the prefill late. And as always, the solution is in the `solution` folder."

*Timebox.*

---

<div class="tag">Solution</div>

## TODO 1: The prefill and the worker

```cpp
const size_t rounded_max_inference_time =
    std::ceil(max_inference_samples / max_block_size) * max_block_size;      // <- fill out
m_prefill = (k_model.m_input_size - max_block_size) + rounded_max_inference_time; // <- fill out
const size_t output_capacity = rounded_max_inference_time + k_model.m_input_size; // <- fill out

m_output = ThreadSafeRingBuffer(output_capacity);
m_output.push(silence.data(), m_prefill);         // given: the output starts late
…                                                 // given: the warm-up
m_worker = std::thread([this] { worker(); });     // <- fill out
```

Note:
- Your worst case from step 4, in samples, rounded up to whole callbacks: the worker's time
- Prefill: buffer adaptation from step 3, 2048 − 512, plus the worker's time
- 8.2 ms → 512, so 2048 samples of silence before the first result
- `m_output`: the worker's time plus one model block; `m_input`: one model block, as in step 3
- Warm-up given: the slow first calls happen here, not in the audio
- All in `prepare`: the ring buffers allocate here, once, so the callback never has to

**Script:**

"Let's look at the solutions together. First we take your worst case from step 4, in samples, and round it up to whole callbacks: that's the time the worker gets. The prefill is the buffer adaptation we had in step 3, 2048 minus 512, plus that time. With 8.2 milliseconds that's 512, so the output starts with 2048 samples of silence. The output ring buffer needs room for the worker's time plus one model block, and the input keeps one model block, like in step 3. The warm-up is given: the slow first calls from step 4 happen here, before the audio starts. And then we start the worker. It's important that all of this happens in `prepare`: the ring buffers allocate their memory here, once, so the callback never has to."

---

<div class="tag">Solution</div>

## TODO 2: The audio thread

<!-- .slide: class="tight-code" -->

```cpp
for (size_t i = 0; i < num_samples; ++i) {
    const bool pushed = m_input.try_push(samples[i]);        // <- fill out
    if (!pushed) { m_unpushed += num_samples - i; break; }  // <- fill out
}
const size_t zeros = std::min(m_unpushed, num_samples);     // given: zeros for those
std::fill_n(samples, zeros, 0.0f);
m_unpushed -= zeros;

while (m_debt > 0) {                                        // pay the debt first
    if (m_output.try_pop(late)) { --m_debt; } else { break; }  // <- fill out
}

for (size_t i = zeros; i < num_samples; ++i) {
    const bool popped = m_debt == 0 && m_output.try_pop(samples[i]);  // <- fill out
    if (!popped) {                                    // late: the rest is zeros, all owed
        std::fill_n(samples + i, num_samples - i, 0.0f);
        m_debt += num_samples - i;                                     // <- fill out
        break;
    }
}
```

Note:
- Step 3's `process_block`, without the engine, one sample at a time
- Push: if the input is full, the worker is a whole block behind. Count the rest, don't wait
- Those never reach the model: the host gets as many zeros, and nothing comes out of `m_output` for them
- Pop: if a sample isn't there yet, or something is still owed, the worker is late: the rest of the callback goes out as zeros, all owed: the debt
- Not just that one sample: the worker may be pushing right now, and the next one would land one place too early
- When the late samples arrive, their place already went out as zeros: take them out, throw them away
- So the stream stays exactly the prefill late, a dropout doesn't shift it
- Not in here: no `new`, no mutex, no engine, no waiting

**Script:**

"The audio thread is step 3's process_block, without the engine, and one sample at a time. We push the host's samples into the input ring buffer. If one doesn't fit, the worker is a whole block behind. We don't wait: we count the rest and stop. Those samples never reach the model, so the host gets as many zeros instead, and nothing is taken out of the output for them.
Then we hand the host its samples from the output ring buffer. If a sample isn't there yet, or we still owe something, the worker is late: the rest of this callback goes out as zeros, and we owe all of those samples: that's the debt. Why the whole rest and not just that one sample? The worker may be pushing at this very moment, and the next sample we'd get would land one place too early. When the late samples finally arrive, their place already went out as zeros, so we take them out and throw them away. That way a dropout is a dropout, and it doesn't make the rest of the stream later.
Notice what's not in here: no `new`, no mutex, no engine, and no waiting."

---

<div class="tag">Solution</div>

## TODO 3: The worker thread

```cpp
while (m_running.load(std::memory_order_acquire)) {
    const bool has_block = m_input.available() >= k_model.m_input_size;  // <- fill out
    if (!has_block) { continue; }                                         // no sleep

    m_input.pop(m_model_block.data(), k_model.m_input_size);              // <- fill out
    m_engine.process(m_model_block.data(), k_model.m_input_size);         // <- fill out

    size_t pushed = 0;  // wait for room, but do not hang at stop
    while (pushed < k_model.m_input_size && m_running.load(std::memory_order_acquire)) {
        if (m_output.try_push(m_model_block[pushed])) { ++pushed; }      // <- fill out
    }
}
```

Note:
- Loops while the running flag is set
- Not a whole block in `m_input` yet: check again, no sleep. Burns a core; part 2 does it better
- A whole block: take it out, run the model, push it into `m_output`
- `m_output` full: try again until there's room, the worker may wait
- But the waiting loop also checks the running flag
- Without it, `stop()` waits forever

**Script:**

"The worker runs in a loop as long as the running flag is set. It checks whether there's a whole model block in the input ring buffer. If not, it checks again right away, without sleeping, because the result is needed soon. That keeps one core busy, and in part 2 we'll see how to do it better. If there is a block, it takes it out, runs the model on it and pushes the result into the output ring buffer, sample by sample. If the output is full, the worker tries again until there's room. Unlike the audio thread, the worker is allowed to wait.
But notice the running flag in that waiting loop. Without it, when we stop, the audio thread doesn't take samples anymore, the ring buffer never gets room, and `stop()` waits forever."

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
- Zero allocations on the audio thread, same stream as step 2, no sanitizer reports
- Audio thread is real-time safe, the model runs beside it
- If possible, run it next to step 5 live
- The host hears it exactly the prefill late: 1536 + your rounded max inference time
- But the model itself answers late too. One number for the host, and a dry signal: next step

**Script:**

"The result: zero allocations on the audio thread, the same stream as in step 2, and the sanitizer doesn't report anything anymore. The audio thread is now real-time safe, and the model runs beside it."
*If possible, run it next to step 5 live.*
"And the host hears it exactly the prefill late: 1536 plus your rounded max inference time. But the model itself answers late too. So what's the one number we tell the host, and how do we line up a dry signal with it? That's our next step: latency."
