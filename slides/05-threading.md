<h1>Step 5<br>Inference on a<br>worker thread</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- Step 4: the engine can't run on the audio thread
- So it gets its own thread: the worker thread

**Script:**

"In step 4 we found out that the engine can't run on the audio thread. The solution was to move the inference to a separate thread. In this step we'll give it its own thread, the worker thread."

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
  <div><strong>Real-time capable</strong><br>Factor 5.4 in a callback, 0.17 on the worker (step 3)</div>
  <div><strong>Real-time safe</strong><br>Allocations and locks leave the audio thread (step 4)</div>
</div>

Note:
- Two threads
- Audio thread keeps its pace, only moves samples
- Two ring buffers as in step 2, now thread-safe, between two threads
- Audio thread pushes into `m_input`, the worker takes 2048 out once they are there
- Worker runs the model, pushes the result into `m_output`, the audio thread pops it
- Worker may allocate and sleep. No per-callback deadline, but a budget (step 6)
- The audio thread never waits for it
- Click: this solves two problems
- Real-time safe, step 4: the engine's allocations and locks leave the audio thread
- Real-time capable, step 3: the factor of 5.4 was against one 1.45 ms callback; the worker has a whole model block, so 0.17

**Script:**

"Here's the idea. We now have two threads. The audio thread keeps its pace: in every callback it only moves samples around, nothing else. It pushes the host's samples into the input ring buffer, `m_input` as in step 2, but now a thread-safe one. Once there are 2048 samples in it, the worker takes them out, runs the model and pushes the result into `m_output`, where the audio thread picks it up.
The worker is allowed to allocate and even to sleep. It doesn't have a per-callback deadline. It does have a budget, and we'll come back to that in step 6. The audio thread never waits for it.

*Click: the two problems.*

So with the worker thread we solve two problems at once. First, real-time safety from step 4: the engine's allocations and locks are no longer on the audio thread. And second, real-time capability from step 3: our forward pass had a factor of 5.4 when it had to fit into one 64-sample callback. Now it no longer runs inside any callback, it has the time of a whole model block, so the factor is 0.17."

---

<div class="tag">Goal</div>

## The goal<br><span class="then">Create two threads.</span>


<div class="statement-points">
  <div><small>1</small>Size the input ring buffer and start the worker</div>
  <div><small>2</small>Hand blocks over on the audio thread</div>
  <div><small>3</small>Run the model on the worker thread and return the results</div>
</div>

Note:
- Goal: set up the two threads
- `prepare`: size `m_input`, start the worker
- Audio thread: samples into `m_input`, samples out of `m_output`
- Worker: take a block out of `m_input`, run the model, push it into `m_output`

**Script:**

"So our goal is to set up these two threads. First we size the input ring buffer and start the worker in `prepare`. Then we write the audio thread's side: samples into `m_input`, samples out of `m_output`. And then the worker's side: take a block out of `m_input`, run the model, push it into `m_output`."

---

<div class="tag">Given</div>

## What we get <span class="then">1 new helper, 1 file to edit.</span>



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
    {{FILE:host.h}}
    {{FILE:threadsafe_ring_buffer.h|new}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">05_threading/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
- Still the model, signals, engine, ring buffer and host
- New: `ThreadSafeRingBuffer`, step 2's ring buffer safe between two threads, on moodycamel, fetched by CMake
- Host now runs in real time: waits between callbacks, 512 samples per call
- `main.cpp` counts allocations: check the audio thread doesn't allocate anymore

**Script:**

"We still have the model, the signals, the engine, the ring buffer and the host from the steps before. New is the `ThreadSafeRingBuffer`: the ring buffer from step 2, with the same functions, but safe between two threads. Underneath is a lock-free buffer from moodycamel, which CMake fetches for us. Our fake host also changed: it now runs in real time, so it waits between the callbacks like a real audio device, with 512 samples per call. And in `main.cpp` there's a counter for allocations, so we can check that the audio thread really doesn't allocate anymore."

---

<div class="tag">Helpers: <code>threadsafe_ring_buffer.h</code></div>

## One writer, one reader<br><span class="then">No locks.</span>

```cpp
ThreadSafeRingBuffer m_input{capacity};   // Same functions as RingBuffer

m_input.space();                          // audio thread: room to push?
m_input.push(samples, num_samples);

m_input.available();                      // worker: a whole block there?
m_input.pop(block, 2048);

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
  <div class="q-fail"><code>space()</code> too small → dropped</div>
</div>

Note:
- Same `push`, `pop`, `available`, `space` as `RingBuffer` in step 2
- One thread writing, one reading: the simplest lock-free case
- Underneath: moodycamel's circular buffer, only its `try_` functions, which never wait and never allocate
- `available` and `space` can only be too small from the other side, never too large: check, then push or pop
- Ring buffer full: audio thread drops the block
- A glitch, but waiting would be a missed deadline for the whole host

**Script:**

"Let's look at the ring buffer. It has the same functions as the one from step 2: push, pop, available and space. What changed is inside. Each buffer has exactly one thread writing and one thread reading, which is the simplest lock-free case there is. Underneath is a circular buffer from moodycamel, and we only use its try functions, which never wait and never allocate. If the input ring buffer is full, the audio thread drops the block. That's a glitch, but waiting would be a missed deadline for the whole host."
  

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="10"></div>

Fill in the TODOs in `steps/05_threading/exercise/main.cpp`

```bash
cmake --build --preset release --target step05_exercise
./build/bin/step05_exercise
```

<div class="run">
  <div class="run-step" data-seconds="2.4">
    <div class="run-title"><span>1</span>Ring buffer and the worker</div>
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
- Three TODOs: `prepare` (input ring buffer + worker), the audio thread, the worker thread
- `main.cpp` in steps/05_threading/exercise, build commands on the slide
- Biggest task so far, but one line at a time: 5 minutes
- When it works: 0 allocations on the audio thread
- Solution is in the `solution` folder

**Script:**

"Now it's your turn. There are three TODOs: first size the input ring buffer and start the worker in `prepare`, the audio thread, and the worker thread. You'll find the `main.cpp` in `steps/05_threading/exercise`, and the build commands are on the slide. This is the biggest task so far, but it's one line at a time, so you get 5 minutes. When it works, the program should report 0 allocations on the audio thread. And as always, the solution is in the `solution` folder."

*Timebox.*

---

<div class="tag">Solution</div>

## TODO 1: Ring buffer and the worker

```cpp
const size_t input_capacity =
    max_block_size + k_blocks_in_flight * k_model.m_input_size;  // <- fill out

m_input = ThreadSafeRingBuffer(input_capacity);

m_running.store(true, std::memory_order_release);
m_worker = std::thread([this] { worker(); }); // <- fill out
```

Note:
- `prepare`: `m_input` with room for eight model blocks, running flag, start the worker
- Step 2 needed one model block: the model ran right away. Now the worker can fall behind
- All in `prepare`: the ring buffers allocate here, once, so the callback never has to

**Script:**

"Let's look at the solutions together. In `prepare` we give the input ring buffer room for eight model blocks, set the running flag and start the worker thread. In step 2 one model block was enough, because the model ran right away. Now the worker can fall behind. It's important that all of this happens in `prepare`: the ring buffers allocate their memory here, once, so the callback never has to."

---

<div class="tag">Solution</div>

## TODO 2: The audio thread

```cpp
const bool has_room = m_input.space() >= num_samples;       // <- fill out
if (has_room) {
    m_input.push(samples, num_samples);                     // <- fill out
} else {
    m_dropped.fetch_add(1);                                 // Worker is behind: drop
}

const bool is_ready = m_output.available() >= num_samples;  // <- fill out
if (is_ready) {
    m_output.pop(samples, num_samples);                     // <- fill out
} else {
    std::fill_n(samples, num_samples, 0.0f);                // Nothing finished yet
}
```

Note:
- Step 2's `process_block`, without the engine
- Room in the input ring buffer: push the host's samples
- No room: the worker is behind, count it as dropped, move on, don't wait
- Like step 2: hand back samples from the output ring buffer, or silence if nothing is ready
- `m_dropped`: how many blocks the worker couldn't keep up with
- Not in here: no `new`, no mutex, no engine

**Script:**

"The audio thread is step 2's process_block, without the engine. If there's room in the input ring buffer, we push the host's samples into it. If there's no room, the worker is behind, so we count the block as dropped and move on. We don't wait.
Then, like in step 2, we hand the host its samples from the output ring buffer, or silence if nothing is ready yet. With m_dropped we can track how many blocks the worker couldn't keep up with.
Notice what's not in here: no `new`, no mutex, and no engine."

---

<div class="tag">Solution</div>

## TODO 3: The worker thread

```cpp
while (m_running.load(std::memory_order_acquire)) {
    const bool has_block = m_input.available() >= k_model.m_input_size;  // <- fill out
    if (!has_block) { sleep_for(100us); continue; }

    m_input.pop(m_model_block.data(), k_model.m_input_size);              // <- fill out
    m_engine.process(m_model_block.data(), k_model.m_input_size);         // <- fill out

    // Wait for room, but do not hang at stop
    while (m_running.load(std::memory_order_acquire) &&
           m_output.space() < k_model.m_input_size) { sleep_for(100us); }
    m_output.push(m_model_block.data(), k_model.m_input_size);            // <- fill out
}
```

Note:
- Loops while the running flag is set
- Not a whole block in `m_input` yet: sleep, try again
- A whole block: take it out, run the model, push it into `m_output`
- Worker may retry and sleep: no hard per-callback deadline
- Given loop also checks the running flag while waiting for room in `m_output`
- Without it, `stop()` waits forever

**Script:**

"The worker runs in a loop as long as the running flag is set. It checks whether there's a whole model block in the input ring buffer. If not, it sleeps for a moment and tries again. If there is, it takes it out, runs the model on it and pushes the result into the output ring buffer. Unlike the audio thread, the worker is allowed to retry and sleep, because the deadline is much looser. It has no hard per-callback deadline.
Notice one thing in the loop we gave you: when the worker waits for room in the output ring buffer, it also checks the running flag. Otherwise, when we stop, the audio thread doesn't take samples anymore, the ring buffer never gets room, and `stop()` waits forever."

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
- Zero allocations on the audio thread, same output as step 1, no sanitizer reports
- Audio thread is real-time safe, the model runs beside it
- If possible, run it next to step 4 live
- But the first blocks are silence again
- How many samples late? How do we tell the host? Next step: latency

**Script:**

"The result: zero allocations on the audio thread, the same output as in step 1, and the sanitizer doesn't report anything anymore. The audio thread is now real-time safe, and the model runs beside it."
*If possible, run it next to step 4 live.*
"But if you look at the output, the first blocks came back as silence again. So how many samples late is our output exactly? And how do we tell the host? In our next step we'll talk about latency."
