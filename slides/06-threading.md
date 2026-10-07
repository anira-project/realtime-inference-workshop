<h1>Step 6<br>Inference on a<br>worker thread</h1>

The engine has to go somewhere — just not here

<!-- .slide: data-state="no-header" -->

Note:
    - Step 5 ended with "not on the audio thread". This is the answer.

---

## The split

```
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

## What crosses the boundary

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

## The callback, in full

```cpp
m_input.push(samples, num_samples);

while (m_input.available() >= k_model.m_input_size) {
    ModelBlock block;
    m_input.pop(block.m_samples.data(), k_model.m_input_size);
    if (!m_to_worker.try_enqueue(block)) { break; }   // Worker is behind
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

## Two checks

```
  allocations on the audio thread 0
  the model produced           6144 of 6144 samples
  max abs diff vs. reference   2.21e-06
```

- Counted with our own `operator new`, **thread-local** — the worker's 11,000
  allocations per block do not count, and should not
- Same output as step 1

Under RTSan: **no report at all.**

Note:
    - Worth running live next to step 5, which stops at the first line of the engine.

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
