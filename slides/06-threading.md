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
- Two threads, on one time axis: every row lines up on the same callbacks
- Audio thread: every 512 samples a callback. It pushes its block into `m_input` and takes one out of `m_output`, nothing else
- `m_input` fills a quarter per callback; at the fourth, a whole model block of 2048 is there and the worker takes it
- Worker thread: it spins on `try_pop` the whole time, and runs the model as soon as a block is whole. It is not tied to the callbacks: the run may take longer than one callback
- `m_output` starts with the prefill, 5 blocks: the host hears silence first. The model's result arrives before the prefill runs out, and from then on the host hears the model's stream
- We solve 2 problems:
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
    {{FILE:host.h|new}}
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
- New: `ThreadSafeRingBuffer`, step 3's ring buffer safe between two threads, on moodycamel, fetched by CMake
- New: `WorkerThread`, `start` and `stop` the worker thread

---

<!-- .slide: data-visibility="hidden" -->

<div class="tag">Helpers: <code>host.h</code></div>

## The host <span class="then">A DAW, minus the DAW, at audio rate.</span>

```cpp
for (size_t start = 0; start < buffer.size(); start += 512) {
    processor.process_block(buffer.data() + start, 512);    // Your callback, in place
    std::this_thread::sleep_for(block_duration(512, 44100)); // New: one block every 11.6 ms
}
```

<div class="streams once">
  {{STREAM:host=512|model=0|length=4096|seconds=5}}
</div>

Note:
- A DAW without the DAW: it calls your callback block by block
- Overwrite the samples in place, like an audio callback
- There is a sleep after every callback, one block of audio, 11.6 ms. `block_duration()` from `host.h`
- Without it the loop calls the audio thread as fast as it can, the worker could never keep up
- Block size is just a parameter: 512 for the whole first part

---

<div class="tag">Helpers: <code>threadsafe_ring_buffer.h</code></div>

## Every callback 512 in, 512 out<br><span class="then">The worker spins until a model block is whole.</span>

<div class="flow">
  <div class="flow-lane" style="top: -48px">audio thread</div>
  <div class="flow-lane" style="top: 162px">worker thread</div>
  <div class="flow-lane-line"></div>
  <div class="flow-host in"></div>
  <div class="q flow-in"><div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div></div>
  <div class="q flow-out"><div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div></div>
  <div class="flow-host out"></div>
  <div class="q flow-block"><div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div></div>
  <div class="flow-model">model</div>
  <div class="flow-name" style="top: 76px; left: 0px">callback: 512 in</div>
  <div class="flow-name" style="top: 76px; left: 220px"><code>m_input</code>, ring buffer</div>
  <div class="flow-name" style="top: 76px; left: 1120px"><code>m_output</code>, ring buffer, the prefill first</div>
  <div class="flow-name" style="top: 76px; left: 1600px">callback: 512 out</div>
  <div class="flow-name" style="top: 266px; left: 420px">model block, 2048</div>
  <div class="flow-say spin" style="top: 236px; left: 0"><span class="flow-spinner"></span><code>try_pop()</code> <span class="blink">false, again</span></div>
  <div class="flow-say push" style="top: 236px; left: 1040px"><code>try_push()</code> the result</div>
  <i class="in"></i>
  <i class="in"></i>
  <i class="in"></i>
  <i class="in"></i>
  <i class="pre"></i>
  <i class="pre"></i>
  <i class="pre"></i>
  <i class="pre"></i>
  <i class="pre"></i>
  <i class="res"></i>
  <i class="res"></i>
  <i class="res"></i>
  <i class="res"></i>
</div>

<div class="cards-note"><code>ThreadSafeRingBuffer</code>: <code>try_push</code> and <code>try_pop</code>, one sample at a time, never wait. One writer, one reader: no locks.</div>

<!-- ```cpp
m_worker.start([this] { process_model(); });  // over and over, until stop()
``` -->

Note:
- Every callback, every 512 samples, the host hands us its own buffer: not a ring buffer, just the block. The audio thread pushes it into `m_input`, `try_push`
- The worker spins: `try_pop` returns false again and again until a sample is there, and takes each one at once, until a whole model block of 2048 is together. That is TODO 3
- Then the model runs, and the worker pushes the result into `m_output`, `try_push`
- Meanwhile the audio thread takes one block out of `m_output` every callback, `try_pop`: first the prefill, silence, then the model's stream. If something is missing, it never waits: a zero goes out. That is TODO 2
- Both rings: one writer, one reader, so no locks. `try_push` and `try_pop` never wait, never allocate, a bool says whether it worked
- Underneath: moodycamel's circular buffer, only its `try_` functions
<!-- - `WorkerThread`: given, calls our `process_model` over and over, from `start` until `stop` -->

---

<div class="tag">The worker's time</div>

## The worker's time<br><span class="then">Your worst case, in whole callbacks.</span>

<div class="lat-add">
  <div class="lat-term"><span>max inference time</span><small>step 4, inference_time.h</small><strong>18.0 ms</strong></div>
  <div class="lat-op">=</div>
  <div class="lat-term fragment" data-fragment-index="1"><span>in samples</span><small>× 44.1 kHz</small><strong>794</strong></div>
  <div class="lat-op fragment" data-fragment-index="2">→</div>
  <div class="lat-term total fragment" data-fragment-index="2"><span>worker_time</span><small>whole 512-sample callbacks</small><strong>1024</strong></div>
</div>

<div class="fragment" data-fragment-index="3">

```cpp
// given, in prepare()
const double worst_case_samples = k_max_inference_ms * k_model.m_sample_rate / 1000.0;
const size_t worker_time =
    static_cast<size_t>(std::ceil(worst_case_samples / max_block_size)) * max_block_size;
```

</div>

Note:
- The worker needs time to run a model block: your worst case from step 4, the number you put into `inference_time.h`
- My worst case: 18 ms
- Click: in samples, at 44.1 kHz: 18 ms are 794 samples
- Whole callbacks, because the audio thread only picks results up once per callback
- Click: rounded up to whole 512-sample callbacks: 1024, two callbacks
- Click: You don't need to calculate this yourself, it's given at the top of `prepare`. TODO 1 builds on `worker_time`

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
- Then the prefill in `prepare`: step 3's, plus the worker's time
- Then set up the audio thread, the worker thread
- `main.cpp` in steps/06_threading/exercise, build commands on the slide
- The biggest task so far, 10 minutes
- Solution is in the `solution` folder

---

<div class="tag">Solution</div>

## TODO 1: The prefill

```cpp
m_prefill = «(k_model.m_input_size - max_block_size)»    // <- solution: step 3's prefill
          + «worker_time»;                               //    plus the worker's time

// given: the sizes, the silence goes in, and the worker thread starts
m_input = ThreadSafeRingBuffer(k_model.m_input_size);
m_output = ThreadSafeRingBuffer(worker_time + k_model.m_input_size);
m_worker.start([this] { process_model(); });
```

Note:
- `m_output` starts with a prefill of zeros, so the host never finds it empty
- The prefill is the sum of:
  - step 3's prefill, the buffer adaptation: one model block minus one host block
  - the worker's time, `worker_time`: the block still has to go through the model. Your worst case from step 4, in whole callbacks
- A wrong prefill: the check says what's wrong
- The sizes are given: `m_input` one model block, `m_output` the worker's time plus one model block, so the prefill fits
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
- Same job as step 3's `process_block`, minus the engine: host samples into `m_input`, model samples out of `m_output`
- You fill in two calls: `try_push` and `try_pop`. They never wait and never allocate, they only say whether it worked
- try_push: `m_input` full? The rest of the block is lost. We count it in `m_unpushed` and send that many zeros instead
- try_pop: `m_output` empty? The worker is late. The rest of this callback goes out as zeros: a dropout
- Those zeros are our debt. When the late samples arrive we throw that many away first, so everything after stays in place: a dropout, but the latency never grows

---

<div class="tag">Solution</div>

## TODO 3: The worker thread

```cpp
// in process_model(), called over and over by m_worker
for (size_t i = 0; i < k_model.m_input_size; ++i) {
    while («!m_input.try_pop(m_model_block[i])») {    // <- solution: spin
        if (!m_worker.running()) { return; }        // given
    }
}
m_engine.process(m_model_block.data(), k_model.m_input_size,
                 m_state.data(), k_model.m_state_size);
for (size_t i = 0; i < k_model.m_input_size; ++i) {
    while («!m_output.try_push(m_model_block[i])») {  // <- solution: spin
        if (!m_worker.running()) { return; }        // given
    }
}
```

Note:
- The other half of step 3's `process_block`: the inference is the same, given
- The worker may wait, the audio thread may not. So here a failed `try_pop` or `try_push` is simply tried again, right away: a hot spin, until the whole model block is in, then until the result is out
- The `running()` check is given: without it `stop()` would wait forever for samples that never come
- Be careful, this is not the best way to do it. That keeps one core busy; part 2 does it better
- The engine may allocate and lock here: nobody waits on this thread

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


---

<!-- .slide: data-visibility="hidden" -->

<div class="tag">Bonus question</div>

## When the worker is late<br><span class="then">A dropout, not a shift.</span>

<svg class="debt-fig" viewBox="0 0 1680 470" aria-label="the host stream when the worker is late, without and with debt">
  <g>
    <text class="row" x="350" y="71">on time</text>
    <rect class="cell" x="380" y="20" width="96" height="84" rx="8"/>
    <text class="num" x="428" y="73">0</text>
    <rect class="cell" x="486" y="20" width="96" height="84" rx="8"/>
    <text class="num" x="534" y="73">1</text>
    <rect class="cell" x="592" y="20" width="96" height="84" rx="8"/>
    <text class="num" x="640" y="73">2</text>
    <rect class="cell" x="698" y="20" width="96" height="84" rx="8"/>
    <text class="num" x="746" y="73">3</text>
    <rect class="cell" x="804" y="20" width="96" height="84" rx="8"/>
    <text class="num" x="852" y="73">4</text>
    <rect class="cell" x="910" y="20" width="96" height="84" rx="8"/>
    <text class="num" x="958" y="73">5</text>
    <rect class="cell" x="1016" y="20" width="96" height="84" rx="8"/>
    <text class="num" x="1064" y="73">6</text>
    <rect class="cell" x="1122" y="20" width="96" height="84" rx="8"/>
    <text class="num" x="1170" y="73">7</text>
    <rect class="cell" x="1228" y="20" width="96" height="84" rx="8"/>
    <text class="num" x="1276" y="73">8</text>
    <rect class="cell" x="1334" y="20" width="96" height="84" rx="8"/>
    <text class="num" x="1382" y="73">9</text>
    <rect class="cell" x="1440" y="20" width="96" height="84" rx="8"/>
    <text class="num" x="1488" y="73">10</text>
    <rect class="cell" x="1546" y="20" width="96" height="84" rx="8"/>
    <text class="num" x="1594" y="73">11</text>
  </g>
  <g class="fragment">
    <text class="row" x="350" y="206">late, no debt</text>
    <text class="row sub" x="350" y="240">everything after: 2 later</text>
    <rect class="cell" x="380" y="170" width="96" height="84" rx="8"/>
    <text class="num" x="428" y="223">0</text>
    <rect class="cell" x="486" y="170" width="96" height="84" rx="8"/>
    <text class="num" x="534" y="223">1</text>
    <rect class="cell" x="592" y="170" width="96" height="84" rx="8"/>
    <text class="num" x="640" y="223">2</text>
    <rect class="zero" x="698" y="170" width="96" height="84" rx="8"/>
    <rect class="zero" x="804" y="170" width="96" height="84" rx="8"/>
    <rect class="cell shifted" x="910" y="170" width="96" height="84" rx="8"/>
    <text class="num" x="958" y="223">3</text>
    <rect class="cell shifted" x="1016" y="170" width="96" height="84" rx="8"/>
    <text class="num" x="1064" y="223">4</text>
    <rect class="cell shifted" x="1122" y="170" width="96" height="84" rx="8"/>
    <text class="num" x="1170" y="223">5</text>
    <rect class="cell shifted" x="1228" y="170" width="96" height="84" rx="8"/>
    <text class="num" x="1276" y="223">6</text>
    <rect class="cell shifted" x="1334" y="170" width="96" height="84" rx="8"/>
    <text class="num" x="1382" y="223">7</text>
    <rect class="cell shifted" x="1440" y="170" width="96" height="84" rx="8"/>
    <text class="num" x="1488" y="223">8</text>
    <rect class="cell shifted" x="1546" y="170" width="96" height="84" rx="8"/>
    <text class="num" x="1594" y="223">9</text>
  </g>
  <g class="fragment">
    <text class="row" x="350" y="356">late, with debt</text>
    <text class="row sub" x="350" y="390">the stream stays in place</text>
    <rect class="cell" x="380" y="320" width="96" height="84" rx="8"/>
    <text class="num" x="428" y="373">0</text>
    <rect class="cell" x="486" y="320" width="96" height="84" rx="8"/>
    <text class="num" x="534" y="373">1</text>
    <rect class="cell" x="592" y="320" width="96" height="84" rx="8"/>
    <text class="num" x="640" y="373">2</text>
    <rect class="zero" x="698" y="320" width="96" height="84" rx="8"/>
    <rect class="zero" x="804" y="320" width="96" height="84" rx="8"/>
    <rect class="cell" x="910" y="320" width="96" height="84" rx="8"/>
    <text class="num" x="958" y="373">5</text>
    <rect class="cell" x="1016" y="320" width="96" height="84" rx="8"/>
    <text class="num" x="1064" y="373">6</text>
    <rect class="cell" x="1122" y="320" width="96" height="84" rx="8"/>
    <text class="num" x="1170" y="373">7</text>
    <rect class="cell" x="1228" y="320" width="96" height="84" rx="8"/>
    <text class="num" x="1276" y="373">8</text>
    <rect class="cell" x="1334" y="320" width="96" height="84" rx="8"/>
    <text class="num" x="1382" y="373">9</text>
    <rect class="cell" x="1440" y="320" width="96" height="84" rx="8"/>
    <text class="num" x="1488" y="373">10</text>
    <rect class="cell" x="1546" y="320" width="96" height="84" rx="8"/>
    <text class="num" x="1594" y="373">11</text>
    <text class="note" x="698" y="444">3 and 4 arrive late: thrown away</text>
  </g>
</svg>

<!-- <div class="cards-note corner-note">The same on the way in: if <code>m_input</code> is full, those samples never reach the model, and the host gets as many zeros: <code>m_unpushed</code>.</div> -->

Note:
- Only if asked, or time is left: the detail behind `m_debt` from TODO 2
- Top: on time, samples arrive in order
- Click: 3 and 4 aren't there yet, the host gets zeros. Pop them later anyway: everything after is 2 samples late, the latency grows
- Click: with debt: count the zeros, throw 3 and 4 away when they arrive. 5 lands where it belongs: a dropout, but no shift
- Same on the way in: `m_input` full, samples lost, counted in `m_unpushed`
- Not perfect: the zeros for `m_unpushed` go out right away while `m_output` still holds older samples, so for about one prefill the stream is shifted, then back in place

