<h1>Step 2<br>Model input size<br>vs. host buffer size</h1>

<!-- .slide: data-state="no-header" -->

Note:
    - Ask the room what buffer sizes they ship with. Collect the spread.

---

## Why: two owners, two block sizes

- **The model's block** comes from the export: 2048 samples. Fixed at trace time.
- **The host's block** comes from the device and the user's settings: 64, 128, 480,
  512, … and it can change while running.

They can match by coincidence, but they are **not** the same thing.

---

## Why: what that means in a callback

```plaintext
host:   |64|64|64|64|64|64|64| ...        32 callbacks
model:  |------ 2048 ------|              1 forward pass
```

- 31 callbacks with **not enough samples** to run the model
- 1 callback that has to run it, and then produce 64 samples
- 480 does not divide 2048 at all

The fix: a **ring buffer** on both sides, because both sides set their own pace.

```plaintext
host in  →  [ ring buffer ]  →  model (2048)  →  [ ring buffer ]  →  host out
```

---

## Goal

The model sees the same stream as in step 1, **whatever block size the host picks**.

1. **Size the ring buffers** in `prepare()` — *what is the worst moment?*
2. **Write the pump** in `process_block()` — *whole model blocks only*
3. **Hand the host its samples back** — *also when nothing is ready yet*

---

## What's given

```plaintext
models/forward_stateful.pt    the model, as in step 1
WORKSHOP_MODEL_PATH           its path, set by CMake → k_model.m_path
common/libtorch_engine.h      the engine from step 1
common/ring_buffer.h          new: a fixed-size FIFO for samples
common/host.h                 new: a fake host that calls you in blocks
common/test_signal.h          the sine from step 1
common/target_signal.h        what the model should produce
common/support.h              report_line(): one result per block size
exercise/main.cpp             ProcessorExample, three TODOs
```

The host runs every block size in `{64, 128, 480, 512, 1024, 2048}`.

---

## New: `RingBuffer`

```cpp
RingBuffer buffer(capacity);           // Allocates once, never again
buffer.available();                    // Samples waiting to be read
buffer.space();                        // Room left for writing
buffer.push(samples, num_samples);     // Append — throws if it does not fit
buffer.pop(samples, num_samples);      // Take from the front — throws if too few
```

One writer, one reader, fixed capacity. Writing it is not the exercise; sizing and
using it is.

---

## New: the fake host

```cpp
std::vector<float> out = run_host(signal, num_samples, block_size,
                                  [&](float* samples, size_t n) { /* your callback */ });
```

- Calls you block by block, **in place**, like a DAW's audio callback
- The block size is a parameter instead of a driver setting — so it runs in CI
- An optional sample rate spaces the calls out in real time — needed from step 6

---

## Task

<div class="task-timer" data-minutes="5"></div>

Fill in the TODOs in `steps/02_host_buffer_size/exercise/main.cpp`

```bash
cmake --build --preset release --target step02_exercise
./build/bin/step02_exercise
```

<div class="nn-flow task-flow">
  <div class="nn-node">fake host<small>64 … 2048 per call</small></div>
  <div class="nn-arrow">→</div>
  <div class="nn-node exercise">ring buffer → model → ring buffer<small><code>ProcessorExample</code></small></div>
  <div class="nn-arrow">→</div>
  <div class="nn-node check">what the model produced<small>vs. <code>target_signal.h</code>, per block size</small></div>
</div>

---

## If you see this: perfect

```plaintext
  host block 64            max abs diff 3.51e-06   OK
  host block 128           max abs diff 3.51e-06   OK
  host block 480           max abs diff 3.51e-06   OK
  ...
OK: the model saw the same stream at every host block size.
```

We check what the **model** produced, not what the host received: the same stream as
in step 1, independent of the host block size.

---

## TODO 1 — ring buffer capacity

```cpp
const size_t capacity = max_block_size + 2 * m_model_input_size;
```

The worst moment: the host has just written a block, the model has just produced one,
and what it produced before is not drained yet.

That happens whenever the host block does not divide the model block — 480.

---

## TODO 2 — the pump

```cpp
m_input.push(samples, num_samples);

while (m_input.available() >= m_model_input_size) {        // Whole blocks only
    m_input.pop(m_block.data(), m_model_input_size);
    m_engine.process(m_block.data(), m_model_input_size);
    m_output.push(m_block.data(), m_model_input_size);
    m_produced.insert(m_produced.end(), m_block.begin(), m_block.end());
}
```

`while`, not `if`: one big host block can complete more than one model block.

---

## TODO 3 — hand the samples back

```cpp
if (m_output.available() >= num_samples) {
    m_output.pop(samples, num_samples);
} else {
    std::fill_n(samples, num_samples, 0.0f);   // Nothing ready yet: silence
}
```

At the start nothing is ready, so the host gets silence. Leaving the buffer alone
would hand the host its own input back.

---

## What broke?

Two things, quietly:

- The host got **silence** at the start — the model cannot answer before it has
  2048 samples. That is latency, and it gets its own step.
- Every callback now either does nothing, or runs a **whole forward pass**.

How long does that take? And how long may it take?

<!-- .slide: data-state="no-footer" -->

Note:
    - Straight into step 3: measure it, then compare with the budget.
