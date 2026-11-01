<h1>Step 5<br>Inference on a<br>worker thread</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
    - The engine has to go somewhere, just not on the audio thread.
    - Step 4 ended with "not on the audio thread". This is the answer.

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
    <div class="lane-queue to"><span>to_worker</span><i></i></div>
    <div class="lane-queue from"><span>from_worker</span><i></i></div>
  </div>
  <div class="lane-label">worker thread</div>
  <div class="lane worker"><div class="lane-run">model</div></div>
</div>

Note:
    - The audio thread keeps its pace: every callback moves samples, nothing else (the even ticks).
    - A whole block goes down to the worker through one queue, the result comes back up through the other.
    - The worker may allocate and sleep. The audio thread never waits for it.
    - This also answers step 3: the forward pass that was 510 % of one callback no longer runs inside a callback.

**Script:**

"Here's the idea. We now have two threads. The audio thread keeps its pace: in every callback it only moves samples around, nothing else. Once it has collected a full block of 2048 samples, it sends that block down to the worker thread through a queue, `to_worker`. The worker runs the model and sends the result back up through a second queue, `from_worker`.
The worker is allowed to allocate and even to sleep. It doesn't have a per-callback deadline. It does have a budget, and we'll come back to that in step 6. The audio thread never waits for it.
And this also answers step 3: the forward pass that was 510 percent of one callback no longer runs inside any callback."

---

<div class="tag">Goal</div>

## The goal<br><span class="then">Create two threads.</span>


<div class="statement-points">
  <div><small>1</small>Set up the queues and start the worker</div>
  <div><small>2</small>Hand blocks over on the audio thread</div>
  <div><small>3</small>Run the model on the worker thread</div>
</div>

Note:
**Script:**

"So our goal is to set up these two threads. First we create the queues and start the worker in `prepare`. Then we write the audio thread's side: samples in, full blocks out to the worker, finished blocks back. And then the worker's side: take a block, run the model, hand it back."

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
    {{FILE:readerwriterqueue.h|new}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">05_threading/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
    - readerwriterqueue.h: moodycamel's lock-free queue, fetched by CMake.
    - host.h now runs in real time: 512 samples per call.
    - Also in main.cpp: ModelBlock, a counting operator new, and WORKSHOP_AUDIO_CALLBACK ([[clang::nonblocking]] where the compiler has it).

**Script:**

"We still have the model, the signals, the engine, the ring buffer and the host from the steps before. New is the `ReaderWriterQueue` from moodycamel, a lock-free queue, which CMake fetches for us. Our fake host also changed: it now runs in real time, so it waits between the callbacks like a real audio device, with 512 samples per call. And in `main.cpp` there's a counter for allocations, so we can check that the audio thread really doesn't allocate anymore."

---

<div class="tag">Helpers: <code>readerwriterqueue.h</code></div>

## One producer, one consumer<br><span class="then">No locks.</span>

```cpp
struct ModelBlock {
    std::array<float, 2048> m_samples{};   // By value
};

moodycamel::ReaderWriterQueue<ModelBlock> m_to_worker{8};
moodycamel::ReaderWriterQueue<ModelBlock> m_from_worker{8};

m_to_worker.try_enqueue(block);       // copy in  — false if full
m_to_worker.try_dequeue(block);       // copy out — false if empty
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
  <div class="q-fail"><code>try_enqueue</code> → false</div>
</div>

Note:
    - Single producer, single consumer: the cheapest lock-free case there is.
    - try_enqueue never grows the queue. It fails, and failing is a decision you can make in 1 µs: the block is dropped, and the host gets silence if nothing came back. Both are glitches; waiting for the worker would be a missed deadline for the entire host, including every other plugin.
    - By value, so nobody has to agree on who frees what.
      - Only the try_ versions. enqueue() without "try" grows the queue when it is full, and growing allocates.
    - Each queue has one writer and one reader: to_worker is written by the audio thread, from_worker by the worker.
    - The audio thread never retries, because there is no time to. The worker may, because it has no per-callback deadline. It still checks the stop flag while it retries.

**Script:**

"Let's look at the queue. Each one has exactly one thread writing and one thread reading, which is the simplest lock-free case there is, and we put whole blocks in by value, so nobody has to agree on who frees the memory. Only use the `try_` functions: plain `enqueue` grows the queue when it's full, and growing allocates. If the queue is full, the audio thread drops the block. That's a glitch, but waiting would be a missed deadline for the whole host."
  

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="5"></div>

Fill in the TODOs in `steps/05_threading/exercise/main.cpp`

```bash
cmake --build --preset release --target step05_exercise
./build/bin/step05_exercise
```

<div class="run">
  <div class="run-step" data-seconds="2.4">
    <div class="run-title"><span>1</span>Queues and the worker</div>
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
**Script:**

"Now it's your turn. There are three TODOs: first prepare the queues and the worker in `prepare`, the audio thread, and the worker thread. You'll find the `main.cpp` in `steps/05_threading/exercise`, and the build commands are on the slide. This is the biggest task so far, but it's one line at a time, so you get 5 minutes. When it works, the program should report 0 allocations on the audio thread. And as always, the solution is in the `solution` folder."

*Timebox.*

---

<div class="tag">Solution</div>

## TODO 1: Queues and the worker

```cpp
const size_t queue_capacity = k_queue_capacity; // <- fill out

m_to_worker = moodycamel::ReaderWriterQueue<ModelBlock>(queue_capacity);
m_from_worker = moodycamel::ReaderWriterQueue<ModelBlock>(queue_capacity);

m_running.store(true, std::memory_order_release);
m_worker = std::thread([this] { worker(); }); // <- fill out
```

Note:
    - All in prepare(): the queues allocate their blocks up front, so the callback never has to.

**Script:**

"Let's look at the solutions together. In `prepare` we create both queues with a fixed capacity, set the running flag and start the worker thread. It's important that all of this happens in `prepare`: the queues allocate their memory here, once, so the callback never has to."

---

<div class="tag">Solution</div>

## TODO 2: The audio thread

```cpp
while (m_input.available() >= k_model.m_input_size) {
    // ...pop a block
    const bool hand_over_block = m_to_worker.try_enqueue(block);    // <- fill out
    if (!hand_over_block) { m_dropped.fetch_add(1); }                // Worker is behind: drop
}

ModelBlock done;
while (m_output.space() >= k_model.m_input_size) {
    const bool got_a_block = m_from_worker.try_dequeue(done);       // <- fill out
    if (!got_a_block) { break; }                                     // Nothing finished yet
    m_output.push(done.m_samples.data(), k_model.m_input_size);     // <- fill out
}
```

Note:
    - No new, no mutex, no engine.
    - If the queue is full, drop the block. The audio thread does not wait.

**Script:**

"The audio thread starts like step 2: we push the host's samples into the input ring buffer. Whenever there's a full block, we take it out and try to put it into the queue to the worker. If the queue is full, we count it as dropped and move on. We don't wait.
Then we pick up every finished block the worker has sent back, as long as there's space in the output ring buffer. And finally, like in step 2, we hand the host its samples, or silence if nothing is ready yet. With m_dropped we can track how many blocks the worker couldn't keep up with.
Notice what's not in here: no `new`, no mutex, and no engine."

---

<div class="tag">Solution</div>

## TODO 3: The worker thread

```cpp
while (m_running.load(std::memory_order_acquire)) {
    const bool got_a_block = m_to_worker.try_dequeue(block);              // <- fill out
    if (!got_a_block) { sleep_for(100us); continue; }

    m_engine.process(block.m_samples.data(), k_model.m_input_size);       // <- fill out

    while (m_running.load(std::memory_order_acquire)) {                   // Do not hang at stop
        const bool hand_back_block = m_from_worker.try_enqueue(block);    // <- fill out
        if (hand_back_block) { break; }
        sleep_for(100us);
    }
}
```

Note:
    - The inner loop retries until the block is handed back, and gives up when m_running goes false.
    - After the loop (given): drain m_to_worker and process what is left.
    - Why the given inner loop checks m_running: without it, the audio thread has stopped calling, and stop() waits forever.

**Script:**

"The worker runs in a loop as long as the running flag is set. It takes a block from the queue. If there's nothing there, it sleeps for a moment and tries again. If it gets a block, it runs the model on it and sends it back. Unlike the audio thread, the worker is allowed to retry and sleep, because the deadline is much looser. It has no hard per-callback deadline.
Notice one thing in the loop we gave you: when the worker waits for space in the output queue, it also checks the running flag. Otherwise, when we stop, the audio thread doesn't take blocks anymore, the queue never gets space, and `stop()` waits forever."

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
    - Counted with our own operator new, thread-local: the worker's allocations per block do not count, and should not. Same output as step 1. Under RTSan: no report at all.
    - Worth running live next to step 4, which stops at the first line of the engine.
    - Open: the first blocks came back as silence. How many samples late, exactly, and how do you tell the host? Step 6.

**Script:**

"The result: zero allocations on the audio thread, the same output as in step 1, and the sanitizer doesn't report anything anymore. The audio thread is now real-time safe, and the model runs beside it."
*If possible, run it next to step 4 live.*
"But if you look at the output, the first blocks came back as silence again. So how many samples late is our output exactly? And how do we tell the host? In our next step we'll talk about latency."
