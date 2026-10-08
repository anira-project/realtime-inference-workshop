<h1>Step 6<br>Inference on a<br>worker thread</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
    - The engine has to go somewhere, just not on the audio thread.
    - Step 5 ended with "not on the audio thread". This is the answer.

---

<div class="tag">Why</div>

## The audio thread moves samples.<br><span class="then">The worker runs the model.</span>

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
    - The worker may take its time, allocate, sleep. The audio thread never waits for it.

---

<div class="tag">Goal</div>

## The model on its own thread.<br><span class="then">The audio thread allocates nothing.</span>

<div class="statement-points">
  <div><small>1</small>Queues and the worker, in <code>prepare()</code></div>
  <div><small>2</small>The audio thread: samples in, blocks out, results back</div>
  <div><small>3</small>The worker: take, run, hand back</div>
</div>

---

<div class="tag">What's given</div>

## 1 new helper, 1 file to edit.

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">models/ · common/assets/</div>
    {{FILE:forward_stateful.pt}}
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
    <div class="given-folder">06_threading/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
    - readerwriterqueue.h: moodycamel's lock-free queue, fetched by CMake.
    - host.h now runs in real time: 512 samples per call.
    - Also in main.cpp: ModelBlock, a counting operator new, and WORKSHOP_AUDIO_CALLBACK ([[clang::nonblocking]] where the compiler has it).

---

<div class="tag">Helpers · <code>readerwriterqueue.h</code></div>

## One producer, one consumer.<br><span class="then">No locks.</span>

```cpp
struct ModelBlock {
    std::array<float, 2048> m_samples{};   // By value
};

moodycamel::ReaderWriterQueue<ModelBlock> m_to_worker{8};
moodycamel::ReaderWriterQueue<ModelBlock> m_from_worker{8};
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

---

## Task

<div class="task-timer" data-minutes="10"></div>

Fill in the TODOs in `steps/06_threading/exercise/main.cpp`

```bash
cmake --build --preset release --target step06_exercise
./build/bin/step06_exercise
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

---

<div class="tag">Solution</div>

## TODO 1 · Queues and the worker

```cpp
m_to_worker = moodycamel::ReaderWriterQueue<ModelBlock>(k_queue_capacity);
m_from_worker = moodycamel::ReaderWriterQueue<ModelBlock>(k_queue_capacity);

m_running.store(true, std::memory_order_release);
m_worker = std::thread([this] { worker(); });
```

Note:
    - All in prepare(): the queues allocate their blocks up front, so the callback never has to.

---

<div class="tag">Solution</div>

## TODO 2 · The audio thread

```cpp
m_input.push(samples, num_samples);
while (m_input.available() >= k_model.m_input_size) {
    ModelBlock block;
    m_input.pop(block.m_samples.data(), k_model.m_input_size);
    if (!m_to_worker.try_enqueue(block)) { m_dropped.fetch_add(1); }   // Worker is behind
}
ModelBlock done;
while (m_output.space() >= k_model.m_input_size && m_from_worker.try_dequeue(done)) {
    m_output.push(done.m_samples.data(), k_model.m_input_size);
}
if (m_output.available() >= num_samples) { m_output.pop(samples, num_samples); }
else                                     { std::fill_n(samples, num_samples, 0.0f); }
```

Note:
    - No new, no mutex, no engine.
    - If the queue is full, drop the block. The audio thread does not wait.

---

<div class="tag">Solution</div>

## TODO 3 · The worker thread

```cpp
while (m_running.load(std::memory_order_acquire)) {
    if (!m_to_worker.try_dequeue(block)) { sleep_for(100us); continue; }
    m_engine.process(block.m_samples.data(), k_model.m_input_size);
    m_produced.insert(m_produced.end(), block.m_samples.begin(), block.m_samples.end());
    while (!m_from_worker.try_enqueue(block)) {
        if (!m_running.load(std::memory_order_acquire)) { break; }   // Do not hang at stop
        sleep_for(100us);
    }
}
```

Note:
    - After the loop: drain m_to_worker and process what is left.
    - Common fail: waiting for room without checking m_running. The audio thread has stopped calling, and stop() waits forever.

---

<div class="tag">Done</div>

## The audio thread is real-time safe.<br><span class="then">The model runs beside it.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
    - Counted with our own operator new, thread-local: the worker's allocations per block do not count, and should not. Same output as step 1. Under RTSan: no report at all.
    - Worth running live next to step 5, which stops at the first line of the engine.
    - Open: the first blocks came back as silence. How many samples late, exactly, and how do you tell the host? Step 7.
