<h1>Step 2<br>Model input size<br>vs. host buffer size</h1>

<!-- Two block sizes that have nothing to do with each other -->

<!-- .slide: data-state="no-header" -->

Note:
    - Ask the room what buffer sizes they ship with. Collect the spread.

---

## Problem: Two owners, two block sizes

- **The model's block** comes from the export: 2048 samples. Fixed at trace time.
- **The host's block** comes from the device, the sample rate in the user's settings: e.g. 64, 128, 480, 512. Can change whilest running.

They can match coincidentally, but they are **not** the same thing.

---

## Problem: What that means in a callback

```
host:   |64|64|64|64|64|64|64| ...        32 callbacks
model:  |------ 2048 ------|              1 forward pass
```

- 31 callbacks with **not enough samples** to run the model
- 1 callback that has to run it, and then produce 64 samples
- e.g. 2048 cannot be divided by a host block of 480

---

## Goal 

Feed a model that only takes 2048 samples from a host with an arbitrary block size:

- **The host** must be able to run at any block size, and change it on the fly.
- **The model** must run at its fixed block size, and produce the right output.

---

## Solution

We need a **ring buffer** on both sides:

```
host in  →  [ ring buffer ]  →  model (2048)  →  [ ring buffer ]  →  host out
```

because both sides set their own pace. 

The buffers are sized in `prepare()`, the way a plugin does it.

---
## Solution: Tools

The RingBuffer class is a simple circular buffer for floats. It has three main methods:

```cpp
RingBuffer m_input{0};  // ring buffer for samples from the host to the model
RingBuffer m_output{0}; // ring buffer for samples from the model to the host

size_t available();     // the amount of samples currently in the buffer
size_t push(const float* samples, size_t num_samples); // adds samples to the buffer
size_t pop(float* samples, size_t num_samples);        // removes samples from the buffer
```

<!-- ## Solution

To move samples from the host to the model, we need a **pump**:

```cpp
m_input.push(samples, num_samples);

while (m_input.available() >= m_model_input_size) {
    m_input.pop(m_block.data(), m_model_input_size);
    m_engine.process(m_block.data(), m_model_input_size);
    m_output.push(m_block.data(), m_model_input_size);
}
``` -->


---

## Task - 5 minutes

1. **Size the buffers** in `prepare()` — *what is the worst moment?*
2. **Write the pump** in `process_block()` — *whole model blocks only*
3. **Hand samples back** to the host - *also when there are no samples available yet*

Run the exercise:

```bash
./build/bin/step02_exercise
```


---

## What we check

What the model produced, not what the host received.

```
  host block 64            max abs diff 2.21e-06   OK
  host block 480           max abs diff 2.21e-06   OK
  host block 2048          max abs diff 2.21e-06   OK
```

Same stream as in step 1, but independent of the host block size.

---

## What broke?

Two things, quietly:

- The host got **silence** at the start — the model cannot answer before it has
  2048 samples. That is latency; we will address it in a later step.
- Every callback now either does nothing, or runs a **whole forward pass**.

How long does that take? And how long may it take?

&rarr; in step 3 we will measure it, and compare with the budget.

Note:
    - Straight into step 3: measure it, then compare with the budget.
