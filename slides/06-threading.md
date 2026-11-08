<h1>Step 6<br>Inference on a<br>worker thread</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- The forward pass cannot stay on the audio thread for two reasons:
1. It's not fast enough
2. It's not safe
- So it gets its own thread: the worker thread

---

<div class="tag">Motivation</div>

## The audio thread moves samples<br><span class="then">The worker runs the model.</span>

{{THREADS}}

<div class="statement-points two fragment">
  <div><strong>Its own clock</strong><br>The worker is not tied to the callbacks: a forward pass may take longer than one (step 4)</div>
  <div><strong>Its own rules</strong><br>The worker may allocate and lock: the audio thread stays real-time safe, never overloaded (step 5)</div>
</div>

Note:
- Now we have 2 threads
- We move the audio between the threads with thread safe ring buffers.
- Audio thread, which only moves the samples. It pushes its whole sample blocks into `m_input` and takes samples out of `m_output`, one at a time, nothing else
- Worker thread spins on `try_pop` the whole time, and runs the model as soon as a whole block is available. 
- Our input ring buffer has the capacity of one model block again, like in step 3
- `m_output` starts with the prefill again, like in step 3, but this time it is bigger than in our synchronous case. We now need to take our max inference time into account. 
- So our output ring buffer needs a larger capacity now.
- Click: With this setup we solve the 2 problems:
- Its own clock, step 4: even a typical forward pass of 13 ms is longer than our 11.6 ms callback. On the worker that is fine, it has the time of the prefill
- Its own rules, step 5: the engine allocates and locks. On the worker that is allowed; the audio thread stays real-time safe and never waits, never overloaded
---

<div class="tag">Goal</div>

## The goal<br><span class="then">Split processor over two threads.</span>


<div class="statement-points">
  <div><small>1</small>Prefill the output and start the worker thread</div>
  <div><small>2</small>Hand over samples from the audio thread</div>
  <div><small>3</small>Run the model on the worker thread and return the results</div>
</div>

Note:
- Goal: step 3's processor on two threads, with a fixed latency
- `prepare`: the prefill for `m_output`, same as in step 3 plus the worker's time; and start the worker thread
- Audio thread: samples into `m_input`, samples out of `m_output`, never waiting
- Worker: take a block out of `m_input`, run the model, push it into `m_output`

---

<div class="tag">Given</div>

## What we get <span class="then">3 new helpers, 1 file to edit.</span>



<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">common/assets/</div>
    {{FILE:model_forward.onnx}}
    {{FILE:test_signal.h}}
    {{FILE:target_signal.h}}
  </div>
  <div class="given-column many">
    <div class="given-label">Helpers</div>
    <div class="given-folder">common/helpers/ · CMake</div>
    {{FILE:onnx_engine.h}}
    {{FILE:threadsafe_ring_buffer.h|new}}
    {{FILE:worker_thread.h|new}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">06_threading/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
- Still the model, signals and engine
- New: `host.h`, how long one block lasts: step 3's host loop sleeps one host block long after every callback, like an audio device
- New: `ThreadSafeRingBuffer`, it has the same API as our ring buffer from step 3. step 3's ring buffer safe between two threads. Underneath is moodycamel’s single-producer single-consumer circular buffer. 
- New: `WorkerThread`: `start()` launches the thread and repeatedly calls our job, `running()` lets the job check whether it should keep going, and stop() waits for the current call to finish and then ends the thread

---

<div class="tag">The worker's time</div>

## Inference time<br><span class="then">Your worst case, in whole callbacks.</span>

<div class="lat-add">
  <div class="lat-term"><span>max inference time</span><small>step 4, inference_time.h</small><strong>18.0 ms</strong></div>
  <div class="lat-op">=</div>
  <div class="lat-term fragment" data-fragment-index="1"><span>in samples</span><small>× 44.1 kHz</small><strong>794</strong></div>
  <div class="lat-op fragment" data-fragment-index="2">→</div>
  <div class="lat-term total fragment" data-fragment-index="2"><span>rounded_max_inference_samples</span><small>whole 512-sample callbacks</small><strong>1024</strong></div>
</div>



</div>

Note:
- To calculate the prefill we need to know our max inference time in samples rounded to whole callbacks.
- My worst case: 18 ms
- Click: in samples, at 44.1 kHz: 18 ms are 794 samples
- Whole callbacks, because the audio thread only picks results up once per callback
- Click: rounded up to whole 512-sample callbacks: 1024, two callbacks
- Click: You don't need to calculate this yourself, it's given at the top of `prepare`.

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="10"></div>

Fill in the TODOs in `steps/06_threading/exercise/main.cpp`

```bash
cmake --build --preset release --target step06_exercise
./build/release/bin/step06_exercise
```

<div class="run">
  <div class="run-step" data-seconds="2.4">
    <div class="run-title"><span>1</span>The prefill</div>
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
      <div class="run-note">no engine</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>3</span>The worker thread</div>
    <div class="run-visual run-worker">
      <div class="q small one"><div class="q-slot"><b></b></div></div>
      <div class="mini-model">model</div>
      <div class="q small one out"><div class="q-slot"><b></b></div></div>
      <div class="run-ok">✓ the model's stream, the prefill late</div>
    </div>
  </div>
</div>

Note:
- First: make sure your number from step 4 into `k_max_inference_ms` in `steps/common/inference_time.h`, if not done yet
- Then define the prefill in `prepare`. This is step 3's, plus the max inference time
- Set up the audio thread.
- Then set up the worker thread.
- `main.cpp` in steps/06_threading/exercise, build commands on the slide
- The biggest task so far, 10 minutes
- Solution is in the `solution` folder

---

<div class="tag">Solution</div>

## TODO 1: The prefill

```cpp
m_prefill = «(m_model.m_input_size - host_block_size)»    // <- solution: step 3's prefill
          + «rounded_max_inference_samples»;                               //    plus the worker's time

// given: the sizes, the silence goes in, and the worker thread starts
m_input = ThreadSafeRingBuffer(m_model.m_input_size);
m_output = ThreadSafeRingBuffer(rounded_max_inference_samples + m_model.m_input_size);
m_worker.start([this] { process_model(); });
```

Note:
- The prefill is the sum of:
  - the same prefill as in step 3, so the buffer adaptation: one model block minus one host block
  - in our asynchronous set up we also need to take inference time into account, so we need an to add `rounded_max_inference_samples`: the block still has to go through the model.
- The sizes of the ring buffers are given: `m_input` one model block, `m_output` the rounded max inference samples plus one model block, so the prefill fits
---

<div class="tag">Solution</div>

## TODO 2: The audio thread

<!-- .slide: class="tight-code" -->

<div class="output-full large">

```cpp
for (size_t i = 0; i < num_samples; ++i) {
    const bool pushed = «m_input.try_push(samples[i])»;        // <- solution
    if (!pushed) { m_unpushed += num_samples - i; break; }  // given: count the rest
}
const size_t zeros = std::min(m_unpushed, num_samples);     // given: zeros for those
std::fill_n(samples, zeros, 0.0f);
m_unpushed -= zeros;

while (m_debt > 0) {                                        // pay the debt first
    if (m_output.try_pop(late)) { --m_debt; } else { break; }  // given
}

for (size_t i = zeros; i < num_samples; ++i) {
    const bool popped = m_debt == 0 && «m_output.try_pop(samples[i])»;  // <- solution
    if (!popped) {                                    // late: the rest is zeros, all owed
        std::fill_n(samples + i, num_samples - i, 0.0f);
        m_debt += num_samples - i;                                     // given
        break;
    }
}
```

</div>

Note:
- Same job as step 3's `process_block`, minus the engine
- host samples are pushed into `m_input` with try_push, one sample at a time
- model samples popped out of `m_output`, try_pop, one sample at a time
- If `try_push` fails: The rest of the block is lost. We count it in `m_unpushed` and write as many zeros into the output buffer.
- If `try_pop` fails: The model is late. We need to catch up on that time later, so we count it in `m_debt`
- This makes sure we stay time aligned, even if the worker is late.

---

<div class="tag">Solution</div>

## TODO 3: The worker thread

```cpp
// in process_model(), called over and over by m_worker
for (size_t i = 0; i < m_model.m_input_size; ++i) {
    while («!m_input.try_pop(m_model_block[i])») {    // <- solution: spin
        if (!m_worker.running()) { return; }        // given
    }
}
m_engine.process(m_model_block.data(), m_model.m_input_size,
                 m_state.data(), m_model.m_state_size);
for (size_t i = 0; i < m_model.m_input_size; ++i) {
    while («!m_output.try_push(m_model_block[i])») {  // <- solution: spin
        if (!m_worker.running()) { return; }        // given
    }
}
```

Note:
- On the worker thread we process our samples
- We try to pop samples from the input ring buffer all the time without waiting until we have one model block.
- Once we have one model block we process the samples and push the results onto the output ring buffer.
- Be careful, this is not the best way to do it. This spins on one CPU core until a full model block has arrived, wasting that core while it waits; part 2 does it better

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
- So we keep the same stream as step 3, but the engine is off the audio thread
- Audio thread is real-time safe, the model runs beside it
