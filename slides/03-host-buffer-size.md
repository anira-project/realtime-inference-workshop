<h1>Step 3<br>Model input size<br>vs. host buffer size</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- let's have a look into the buffer sizes.

---

<div class="tag">Motivation</div>

## What is the problem? <span class="then">Two owners, two block sizes.</span>

<div class="streams">
  {{STREAM:host=512|model=2048|length=4096|seconds=7}}
</div>

<div class="cards-note corner-note">Part 1 assumes a constant host block that divides 2048: 512. Every other case comes in part 2.</div>

Note:
- Two owners, two block sizes to deal with
- Model: fixed at export
- Host: depends on device and user, can change while running
- They can match by coincidence, we can't count on it
- Part 1 of the workshop assumes the host block is constant and divides 2048: we chose 512
- Blocks that change while running, or are multiples of 2048: part 2, with anira

---

<div class="tag">Approach</div>

## The approach<br><span class="then">A ring buffer on each side.</span>

<span> The host and the model each work in their own block size.</span>

<div class="pump" style="margin-bottom: 250px;">
  <div class="pump-node">host in<small>512 per call</small></div>
  <div class="pump-arrow"></div>
  <div class="pump-buffer in"><div class="pump-fill"></div><div class="pump-mark"></div></div>
  <div class="pump-arrow"></div>
  <div class="pump-node model">model<small>2048 at once</small></div>
  <div class="pump-arrow"></div>
  <div class="pump-buffer out"><div class="pump-fill"></div><div class="pump-mark"></div></div>
  <div class="pump-arrow"></div>
  <div class="pump-node">host out<small>512 per call</small></div>
</div>

Note:
- we need a reliable way of passing the samples, that makes sure the model always get a block of 2048 and the host will always get 512 samples
- The approach: A ring buffer on each side of the model
- Host pushes whatever it has into the first ringbuffer
- The model takes a full block, as soon as it's available. 
- The model processes and writes 2048 to the output ring buffer

---

<div class="tag">Goal</div>

## The goal<br><span class="then">A 512-sample host, a 2048-sample model.</span>

<div class="statement-points">
  <div><small>1</small>Prefill the output ring buffer</div>
  <div><small>2</small>Pump whole blocks through the model</div>
  <div><small>3</small>Hand the host its samples back</div>
</div>

Note:
- Goal: make it work with a the model gets 2048, the host gets its 512 back
- Prefill the output ring buffer, so the host always finds samples
- Pump whole blocks through the model
- Hand the samples back to the host

---

<div class="tag">Given</div>

## What we get<br><span class="then">1 new helper, 1 file to edit.</span>

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">common/assets/</div>
    {{FILE:model_forward.onnx}}
    {{FILE:test_signal.h}}
    {{FILE:target_signal.h}}
  </div>
  <div class="given-column">
    <div class="given-label">Helpers</div>
    <div class="given-folder">common/helpers/</div>
    {{FILE:onnx_engine.h}}
    {{FILE:ring_buffer.h|new}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">03_host_buffer_size/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
- Same model, signals and engine helper
- New: a ring buffer
- The exercise isn't writing a ring buffer, it's sizing and using it

---

<div class="tag">Helpers: <code>ring_buffer.h</code></div>

## The ringbuffer <span class="then">3 methods are given</span>

```cpp
RingBuffer buffer(capacity);           // Allocates once, never again
buffer.available();                    // Samples waiting to be read
buffer.try_push(sample);               // Append at the back — false if full
buffer.try_pop(sample);                // Take from the front — false if empty
```

<div class="ring">
  <div class="ring-slots"></div>
  <div class="ring-fill"></div>
  <div class="ring-head pop">pop</div>
  <div class="ring-head push">push</div>
</div>

Note:
- We use 3 methods: 
- The available() method checks if there are samples in the buffer
- `try_push` at the back, `try_pop` from the front, one sample at a time: first in, first out method
- `try` means:
- They never throw: they return a boolian telling us whether it worked. Even if we try to push and the RB is full or the other way around, nothing changes

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="8"></div>

Fill in the TODOs in `steps/03_host_buffer_size/exercise/main.cpp`

```bash
cmake --build --preset release --target step03_exercise
./build/release/bin/step03_exercise
```

<div class="run">
  <div class="run-step" data-seconds="2.2">
    <div class="run-title"><span>1</span>Prefill the output</div>
    <div class="run-visual run-size-buffers">
      <div class="mini-buf grow"></div>
      <div class="mini-buf grow"></div>
      <div class="run-size">? samples</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3.4">
    <div class="run-title"><span>2</span>Pump the whole blocks</div>
    <div class="run-visual">
      <div class="mini-buf pumping"><div class="mini-fill"></div><div class="mini-mark"></div></div>
      <div class="mini-model">model</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>3</span>Hand back the samples </div>
    <div class="run-visual run-wave run-out">
      <div class="run-reveal">
        <div class="run-silence"></div>
        {{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=120|from=2|to=6}}
      </div>
      <div class="run-ok">✓ the host gets the model's stream back</div>
    </div>
  </div>
</div>

Note:
- Your task is to work out the prefill, pump a whole block, hand the samples back
- `main.cpp` in steps/03_host_buffer_size/exercise, build commands on the slide
- 8 minutes, don't worry if you don't finish, we solve it together

---

<div class="tag">Solution</div>

## TODO 1: The prefill

```cpp
const size_t prefill = «m_model.m_input_size - host_block_size»;  // <- solution

m_input = RingBuffer(m_model.m_input_size);              // given
m_output = RingBuffer(m_model.m_input_size);   // given
for (size_t i = 0; i < prefill; ++i) {                   // given: the zeros go in
    const bool pushed = m_output.try_push(0.0f);
    assert(pushed);  // never fails: m_output has room for the prefill
}
```

Note:
- The host takes 512 samples every callback, but the model's first block is only complete in the fourth callback
- Callbacks 1 to 3 take 3 × 512 = 1536 samples from `m_output`: 2048 − 512. That is the prefill, zeros
- So the host always finds samples, from the very first callback
- The sizes are given: `m_input` one model block, `m_output` one model block
- For host sizes that don't divide 2048 this isn't true anymore, but we'll get into that in part 2

---

<div class="tag">Solution</div>

## TODO 2: Pump whole blocks <span class="then">Into the input buffer.</span>

```cpp
for (size_t i = 0; i < num_samples; ++i) {
    const bool pushed = m_input.«try_push(samples[i])»;  // <- 1. solution
    assert(pushed);  // never fails
}

if (m_input.available() >= «m_model.m_input_size») {  // <- 2. solution
    // a whole model block is there: through the model, next slide
}
```

Note:
- First we push the host's samples into the input ring buffer, sample by sample
- `try_push` never fails: the model takes a whole block out before `m_input` is full. So we only assert it
- Check if 2048 samples are available -> only pump one model block at a time

---

<div class="tag">Solution</div>

## TODO 2: Pump whole blocks <span class="then">Through the model.</span>

```cpp
if (m_input.available() >= m_model.m_input_size) {
    for (size_t i = 0; i < m_model.m_input_size; ++i) {
        const bool popped = «m_input.try_pop(m_model_block[i])»;    // <- 3. solution
        assert(popped);  // never fails: a whole block is there
    }
    m_engine.process(m_model_block.data(), m_model.m_input_size,
                     m_state.data(), m_model.m_state_size);
    for (size_t i = 0; i < m_model.m_input_size; ++i) {
        const bool pushed = «m_output.try_push(m_model_block[i])»;  // <- 3. solution
        assert(pushed);  // never fails: m_output has room
    }
}
```

Note:
- Then: pop, process, push to the output ring buffer
- Those two can't fail: the check said a whole block is there, and `m_output` has room for one. So we only assert it

---

<div class="tag">Solution</div>

## TODO 3: Hand the samples back

```cpp
for (size_t i = 0; i < num_samples; ++i) {
    const bool popped = «m_output.try_pop(samples[i])»;  // <- solution
    assert(popped);  // never fails: the prefill covers the wait
}
```

Note:
- Hand the samples back to the host, sample by sample, straight from the output buffer
- At the start they are the prefill's zeros: `try_pop` never fails, so we only assert it

---

<div class="tag">Bonus question</div>

## What is the algorithmic latency?

<div class="lat-add">
  <div class="lat-term fragment"><span>model block</span><small>samples</small><strong>2048</strong></div>
  <div class="lat-op fragment">−</div>
  <div class="lat-term fragment"><span>host block</span><small>samples</small><strong>512</strong></div>
  <div class="lat-op fragment">=</div>
  <div class="lat-term total fragment"><span>buffer adaptation</span><small>samples, the prefill</small><strong>1536</strong></div>
</div>

<div class="cards-note corner-note fragment">For 512 into 2048. Other host block sizes give other numbers: part 2.</div>

Note:
- Ask first: how much latency does our processing add, the way we built it?
- The model needs 2048, the host brings 512 at a time
- Callbacks 1 to 3: `m_input` only collects samples, the host gets the prefill
- Callback 4: the block is complete, the model runs inside the callback
- Click: that wait is the buffer adaptation, model block minus host block: 1536 samples, about 35 ms at 44.1 kHz. It is our prefill from TODO 1
- In general: model block minus the largest block that divides both, here 2048 − 512
- It is the algorithm's latency, from how we feed the model. The model itself adds its own on top: step 7
- Only for 512: other host block sizes give other numbers, part 2

---

<div class="tag">Done</div>

## The model runs on the host's 512-sample blocks <span class="then">But is it real-time capable?</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
- Our model now runs on the host's 512-sample blocks
- But we now have three callbacks doing nothing, the fourth a full forward pass
- We need to know if our forward pass is fast enough to run in one callback
