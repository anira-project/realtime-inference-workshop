<h1>Step 2<br>Model input size<br>vs. host buffer size</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
    - Ask the room what buffer sizes they ship with. Collect the spread.

---

<div class="tag">Motivation</div>

## What is the problem? <span class="then">Two owners, two block sizes.</span>

<div class="streams">
  {{STREAM:host=64|model=2048|length=4096|seconds=7}}
  {{STREAM:host=441|model=2048|length=4096|seconds=7}}
</div>

Note:
    - The model's block comes from the export: 2048 samples, fixed at trace time.
    - The host's block comes from the device and the user's settings: 64, 128, 441, 512, … and it can change while running.
    - 64: 31 callbacks wait, the 32nd runs the whole model (dark). 441 does not divide 2048 at all: the model runs mid-callback, at a different place every time.
    - They can match by coincidence, but they are not the same thing.

---

<div class="tag">Approach</div>

## The approach<br><span class="then">A ring buffer on each side.</span>

<span> The host and the model each work in their own block size.</span>

<div class="pump" style="margin-bottom: 250px;">
  <div class="pump-node">host in<small>441 per call</small></div>
  <div class="pump-arrow"></div>
  <div class="pump-buffer in"><div class="pump-fill"></div><div class="pump-mark"></div></div>
  <div class="pump-arrow"></div>
  <div class="pump-node model">model<small>2048 at once</small></div>
  <div class="pump-arrow"></div>
  <div class="pump-buffer out"><div class="pump-fill"></div><div class="pump-mark"></div></div>
  <div class="pump-arrow"></div>
  <div class="pump-node">host out<small>441 per call</small></div>
</div>

Note:
    - Input side: the host writes whatever it has; the model runs only once a whole block (the dashed line) is in.
    - Output side: the model writes 2048 at once; the host takes its 441 per call.

---

<div class="tag">Goal</div>

## The goal<br><span class="then">The model should work with any host block size.</span>

<div class="statement-points">
  <div><small>1</small>Size the ring buffers in <code>prepare()</code></div>
  <div><small>2</small>Pump whole model blocks in <code>process_block()</code></div>
  <div><small>3</small>Hand the host its samples back</div>
</div>

---

<div class="tag">Given</div>

## What we get<br><span class="then">1 new helpers, 1 file to edit.</span>

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">models/ & common/assets/</div>
    {{FILE:model.pt}}
    {{FILE:test_signal.h}}
    {{FILE:target_signal.h}}
  </div>
  <div class="given-column">
    <div class="given-label">Helpers</div>
    <div class="given-folder">common/helpers/</div>
    {{FILE:libtorch_engine.h}}
    {{FILE:ring_buffer.h|new}}
    {{FILE:host.h|new}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">02_host_buffer_size/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
    - Model, engine and signals are the ones from step 1.
    - main.cpp holds ProcessorExample with three TODOs; the host runs it at every block size in {64, 128, 441, 512, 1024, 2048}.

---

<div class="tag">Helpers: <code>ring_buffer.h</code></div>

## The ringbuffer <span class="then">A fixed-size queue for samples.</span>

```cpp
RingBuffer buffer(capacity);           // Allocates once, never again
buffer.available();                    // Samples waiting to be read
buffer.space();                        // Room left for writing
buffer.push(samples, num_samples);     // Append at the back — throws if it does not fit
buffer.pop(samples, num_samples);      // Take from the front — throws if too few
```

<div class="ring">
  <div class="ring-slots"></div>
  <div class="ring-fill"></div>
  <div class="ring-head pop">pop</div>
  <div class="ring-head push">push</div>
</div>

Note:
    - One writer, one reader, fixed capacity. Writing it is not the exercise; sizing and using it is.

---

<div class="tag">Helpers: <code>host.h</code></div>

## The host <span class="then">A DAW, minus the DAW.</span>

```cpp
std::vector<float> out = run_host(signal, num_samples, block_size,
                                  [&](float* samples, size_t n) { /* your callback */ });
```

<div class="streams">
  {{STREAM:host=64|model=0|length=4096|seconds=5}}
  {{STREAM:host=441|model=0|length=4096|seconds=5}}
  {{STREAM:host=2048|model=0|length=4096|seconds=5}}
</div>

Note:
    - Calls you block by block, in place, like a DAW's audio callback.
    - The block size is a parameter instead of a driver setting, so it runs in CI.
    - An optional sample rate spaces the calls out in real time; needed from step 6.

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="5"></div>

Fill in the TODOs in `steps/02_host_buffer_size/exercise/main.cpp`

```bash
cmake --build --preset release --target step02_exercise
./build/bin/step02_exercise
```

<div class="run">
  <div class="run-step" data-seconds="2.2">
    <div class="run-title"><span>1</span>Size the ring buffers</div>
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
      <div class="run-ok">✓ the same stream at every block size</div>
    </div>
  </div>
</div>

---

<div class="tag">Solution</div>

## TODO 1: Size the ring buffers

```cpp
const size_t capacity = max_block_size + 2 * m_model_input_size;
```

Note:
    - Size for the worst moment: the host has just written a block, the model has just produced one, and what it produced before is not drained yet.
    - That happens whenever the host block does not divide the model block: 441.

---

<div class="tag">Solution</div>

## TODO 2: Pump whole blocks

```cpp
m_input.push(samples, num_samples);

while (m_input.available() >= m_model_input_size) {        // Whole blocks only
    m_input.pop(m_block.data(), m_model_input_size);
    m_engine.process(m_block.data(), m_model_input_size);
    m_output.push(m_block.data(), m_model_input_size);
    m_produced.insert(m_produced.end(), m_block.begin(), m_block.end());
}
```

Note:
    - while, not if: one big host block can complete more than one model block.

---

<div class="tag">Solution</div>

## TODO 3: Hand the samples back

```cpp
if (m_output.available() >= num_samples) {
    m_output.pop(samples, num_samples);
} else {
    std::fill_n(samples, num_samples, 0.0f);   // Nothing ready yet: silence
}
```

Note:
    - At the start nothing is ready, so the host gets silence. Leaving the buffer alone would hand the host its own input back.

---

<div class="tag">Done</div>

## The model runs on any host block size

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
    - The check compares what the model produced, not what the host received: the same stream as in step 1, at every block size.
    - Open: the output starts with silence. That is latency, and it gets its own step.
    - Every callback does nothing, or a whole forward pass. How long does that take, and how long may it take? Straight into step 3.
