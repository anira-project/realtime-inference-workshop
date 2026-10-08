<h1>Step 6<br>Inference on a<br>worker thread</h1>

The engine has to go somewhere — just not here

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
    - Step 5 ended with "not on the audio thread". This is the answer.

---

## Why: the split

```plaintext
audio thread                     worker thread
------------                     -------------
process_block()                  while (running)
  ring buffer in                   try_dequeue  ←── to_worker
  try_enqueue  ──→ to_worker       engine.process()
  try_dequeue  ←── from_worker     try_enqueue  ──→ from_worker
  ring buffer out
```

The audio thread moves samples. The worker runs the model, and may allocate all
it likes.

---

## Goal

The model runs on a worker thread, and the audio thread **neither allocates nor
locks** — with the same output as step 1.

1. **Give the queues a capacity and start the worker** in `prepare()`
2. **The audio thread:** samples in, whole blocks to the worker, results back to the host
3. **The worker thread:** take a block, run the engine, hand it back — and drain at the end

---

## What's given

```plaintext
models/forward_stateful.pt    the model, as in step 1
WORKSHOP_MODEL_PATH           its path, set by CMake → k_model.m_path
common/libtorch_engine.h      the engine from step 1
common/ring_buffer.h          from step 2
common/host.h                 from step 2, now in real time: 512 samples per call
readerwriterqueue.h           new: moodycamel's lock-free queue, fetched by CMake
exercise/main.cpp             ThreadedProcessor, three TODOs
```

Also in `main.cpp`: `ModelBlock`, a counting `operator new`, and
`WORKSHOP_AUDIO_CALLBACK` — `[[clang::nonblocking]]` where the compiler has it.

---

## New: the lock-free queue

```cpp
struct ModelBlock {
    std::array<float, 2048> m_samples{};   // By value
};

moodycamel::ReaderWriterQueue<ModelBlock> m_to_worker{8};
moodycamel::ReaderWriterQueue<ModelBlock> m_from_worker{8};
```

- **Single producer, single consumer** — the cheapest lock-free case there is
- **`try_enqueue` never grows the queue.** It fails, and failing is a decision you can make in 1 µs
- **By value**, so nobody has to agree on who frees what

---

## Task

<div class="task-timer" data-minutes="10"></div>

Fill in the TODOs in `steps/06_threading/exercise/main.cpp`

```bash
cmake --build --preset release --target step06_exercise
./build/bin/step06_exercise
```

<div class="nn-flow task-flow">
  <div class="nn-node">fake host<small>512 samples, in real time</small></div>
  <div class="nn-arrow">⇄</div>
  <div class="nn-node exercise">audio thread<small>ring buffers, queues</small></div>
  <div class="nn-arrow">⇄</div>
  <div class="nn-node exercise">worker thread<small>engine</small></div>
  <div class="nn-arrow">→</div>
  <div class="nn-stack">
    <div class="nn-node check">0 allocations<small>on the audio thread</small></div>
    <div class="nn-node check">output<small>vs. <code>target_signal.h</code></small></div>
  </div>
</div>

---

## If you see this: perfect

```plaintext
  allocations on the audio thread 0
  the model produced           16384 of 16384 samples
  max abs diff vs. reference   3.51e-06

OK: inference ran on the worker thread, the audio thread only moved samples.
```

- Counted with our own `operator new`, **thread-local** — the worker's 11,000
  allocations per block do not count, and should not
- Same output as step 1. Under RTSan: **no report at all.**

Note:
    - Worth running live next to step 5, which stops at the first line of the engine.

---

## TODO 1 — queues and the worker

```cpp
m_to_worker = moodycamel::ReaderWriterQueue<ModelBlock>(k_queue_capacity);
m_from_worker = moodycamel::ReaderWriterQueue<ModelBlock>(k_queue_capacity);

m_running.store(true, std::memory_order_release);
m_worker = std::thread([this] { worker(); });
```

All in `prepare()`: the queues allocate their blocks up front, so the callback never
has to.

---

## TODO 2 — the audio thread

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

No `new`, no mutex, no engine.

---

## TODO 3 — the worker thread

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
while (m_to_worker.try_dequeue(block)) { /* process what is left */ }
```

Common fail: waiting for room **without** checking `m_running` — the audio thread has
stopped calling, and `stop()` waits forever.

---

## And when the worker is late?

Two places where it can go wrong, and both are decided in the callback:

- `try_enqueue` fails → the block is dropped
- `try_dequeue` finds nothing → the host gets silence

Both are glitches. The alternative — waiting for the worker — is a missed
deadline for the entire host, including every other plugin.

---

## What broke?

Nothing is unsafe any more. But the first blocks came back as **silence**: the
worker cannot answer before it has been handed something.

How much later is the output, exactly? And how do you tell the host, so it can
line the dry signal up with it?

<!-- .slide: data-state="no-footer" -->

Note:
    - Next: latency, and mixing dry with wet without a phase mess.
