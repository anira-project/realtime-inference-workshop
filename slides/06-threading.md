<h1>Step 6<br>Inference on a<br>worker thread</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- The forward pass cannot stay on the audio thread for two reasons:
- 1. It's not fast enough
- 2. It's not safe
- So it gets its own thread: the worker thread

---

<div class="tag">Motivation</div>

## The audio thread moves samples<br><span class="then">The worker runs the model.</span>

<div class="lanes">
  <div class="lane-label">audio thread</div>
  <div class="lane audio"></div>
  <div class="lane-label"></div>
  <div class="lane-gap">
    <div class="lane-queue to"><span>m_input</span><i></i></div>
    <div class="lane-queue from"><span>m_output</span><i></i></div>
  </div>
  <div class="lane-label">worker thread</div>
  <div class="lane worker"><div class="lane-run">model</div></div>
</div>

<div class="statement-points two fragment">
  <div><strong>Real-time capable</strong><br>The worker gets its own time: your worst case (step 4)</div>
  <div><strong>Real-time safe</strong><br>Allocations and locks leave the audio thread (step 5)</div>
</div>

Note:
- Two threads
- Audio thread keeps its pace, only moves samples
- How does the audio get there and back, within the rules?
- Two ring buffers as in step 3, now thread-safe, between two threads
- Audio thread pushes into `m_input`, the worker takes 2048 out once they are there
- Worker runs the model, pushes the result into `m_output`, the audio thread pops it
- Worker may allocate. No per-callback deadline, but a budget: your rounded max inference time from step 4
- The audio thread never waits for it
- Click: this solves our two problems
- Real-time capable, step 4: even a typical call of 13 ms is longer than our host callback; the worker isn't inside any callback, it gets its own time
- Real-time safe, step 5: the engine's allocations and locks leave the audio thread

---

<div class="tag">Goal</div>

## The goal<br><span class="then">Split processor over two threads.</span>


<div class="statement-points">
  <div><small>1</small>Size the ring buffers and start the worker thread</div>
  <div><small>2</small>Hand over samples from the audio thread</div>
  <div><small>3</small>Run the model on the worker thread and return the results</div>
</div>

Note:
- Goal: step 3's processor on two threads, with a fixed latency
- `prepare`: size `m_input` and `m_output`; and start the worker thread
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
- New: `host.h`, how long one block lasts: step 3's host loop sleeps that long after every callback, like an audio device
- New: `ThreadSafeRingBuffer`, step 3's ring buffer safe between two threads, on moodycamel, fetched by CMake
- New: `WorkerThread`, `start` and `stop` the worker thread

---

<div class="tag">Helpers: <code>host.h</code></div>

## The host <span class="then">A DAW, minus the DAW, at audio rate.</span>

```cpp
for (size_t start = 0; start < output.size(); start += 512) {
    processor.process_block(output.data() + start, 512);    // Your callback, in place
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

## One writer, one reader<br><span class="then">No locks.</span>

<table class="budget-table rb-table">
  <thead><tr><th></th><th>Audio thread <small>one sample, never waits</small></th><th>Worker <small>whole blocks</small></th></tr></thead>
  <tbody>
    <tr><td><code>m_input</code></td><td><code>try_push(sample)</code> <small>false if full</small></td><td> <code>pop(block, block_size)</code> & <code>available()</code></td></tr>
    <tr><td><code>m_output</code></td><td><code>try_pop(sample)</code> <small>false if empty</small></td><td><code>push(block, block_size)</code></td></tr>
  </tbody>
</table>

<div class="queue-demo" style="margin-bottom: var(--space-8)">
  <div class="q">
    <div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div><div class="q-slot"></div>
  </div>
  <i class="q-chip" style="--n:0"></i>
  <i class="q-chip" style="--n:1"></i>
  <i class="q-chip" style="--n:2"></i>
  <i class="q-chip" style="--n:3"></i>
  <i class="q-chip bounce"></i>
  <div class="q-fail"><code>try_push()</code> fails → a zero goes out instead</div>
</div>

```cpp
m_worker.start([this] { process_model(); });  // over and over, until stop()
```

Note:
- Each ring buffer: one thread writes, the other reads. Read the table by row
- Thread-safe: the writer only moves the write position, the reader only the read position. Nothing is written by both, so no lock
- Atomics with ordering: the sample is written before the position is published
- Audio thread: `try_push` / `try_pop`, one sample, a bool says whether it worked
- Worker: `available`, `pop`, `push` for whole blocks; `pop` takes nothing and returns false if too few are there (step 3's throws)
- Underneath: moodycamel's circular buffer, only its `try_` functions: never wait, never allocate, never throw
- `WorkerThread`: given, calls our `process_model` over and over, from `start` until `stop`

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="10"></div>

Fill in the TODOs in `steps/06_threading/exercise/main.cpp`

```bash
cmake --build --preset release --target step06_exercise
./build/bin/step06_exercise
```

<div class="run">
  <div class="run-step" data-seconds="2.4">
    <div class="run-title"><span>1</span>Give each ring buffer a size
  </div>
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
- three TODOs: the ring buffer sizes in `prepare`, the audio thread, the worker thread
- `main.cpp` in steps/06_threading/exercise, build commands on the slide
- The biggest task so far, one line at a time: 10 minutes
- When it works: the host hears the model's stream, exactly the prefill late
- Solution is in the `solution` folder

---

<div class="tag">Solution</div>

## TODO 1: The ring buffer sizes

```cpp
// given: how much silence m_output starts with
adaptation = (k_model.m_input_size - max_block_size);
m_prefill = adaptation + k_max_inference_samples_rounded;

const size_t input_capacity = k_model.m_input_size;  // <- fill out
const size_t output_capacity =
    k_max_inference_samples_rounded + k_model.m_input_size;  // <- fill out

// given: the silence goes in, and the worker thread starts
m_output.push(silence.data(), m_prefill);
m_worker.start([this] { process_model(); });
```

Note:
- `m_input`: one model block, as in step 3: the worker takes it out as soon as it's full
- Given: the prefill, the silence `m_output` starts with, so the host never finds it empty. Two waits:
  - `adaptation`: 2048 − 512 = 1536. Callbacks 1–3, while the first block fills up, as in step 3
  - the worker's time: then the block still has to go through the model. Your worst case from step 4, rounded up to whole callbacks
- Together 2560: that's how late the host hears the stream
- `m_output`: the worker's time plus one model block. Fullest moment: a fast worker pushes a new block while the worker's time is still waiting in there

---

<div class="tag">Solution</div>

## TODO 2: The audio thread

<!-- .slide: class="tight-code" -->

<div class="output-full large">

```cpp
for (size_t i = 0; i < num_samples; ++i) {
    const bool pushed = m_input.try_push(samples[i]);        // <- fill out
    if (!pushed) { m_unpushed += num_samples - i; break; }  // given: count the rest
}
const size_t zeros = std::min(m_unpushed, num_samples);     // given: zeros for those
std::fill_n(samples, zeros, 0.0f);
m_unpushed -= zeros;

while (m_debt > 0) {                                        // pay the debt first
    if (m_output.try_pop(late)) { --m_debt; } else { break; }  // given
}

for (size_t i = zeros; i < num_samples; ++i) {
    const bool popped = m_debt == 0 && m_output.try_pop(samples[i]);  // <- fill out
    if (!popped) {                                    // late: the rest is zeros, all owed
        std::fill_n(samples + i, num_samples - i, 0.0f);
        m_debt += num_samples - i;                                     // given
        break;
    }
}
```

</div>

Note:
- Step 3's `process_block`, without the engine, one sample at a time
- To fill in: only `try_push` and `try_pop`. Both never wait, they say whether it worked. This way we don't allocate.
- Push: if a sample doesn't fit, count the rest in `m_unpushed` and stop
- Pop: if a sample isn't there, the rest of the callback goes out as zeros
- To understand this better, let's have a look at what happens if a block is actually late

---

<div class="tag">Shift</div>

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
- What if the worker is late anyway? because of one slow call e.g.
- Top: what goes in: on time, samples arrive in order
- Click: 3 and 4 aren't there yet, so the host gets silence. A dropout, unavoidable
- Naive: pop them later. Then everything after is 2 samples late, and the latency grows
- Click: with a debt. Count the zeros, throw 3 and 4 away when they arrive
- 5 lands where it belongs: a dropout, but no shift
- Same on the way in: `m_input` full, samples lost, counted in `m_unpushed`
- A safety net: the dropout stays, but the latency doesn't grow
- Not perfect: the zeros for `m_unpushed` go out right away, while `m_output` still holds the samples from before them. For about one prefill the stream is shifted by those zeros, then it's back in place. To put the zeros in their right place, the audio thread would have to write them into `m_output`, but the worker is its only writer

---

<div class="tag">Solution</div>

## TODO 3: The worker thread

```cpp
void process_model() {  // called over and over by m_worker
    const bool has_block = m_input.available() >= k_model.m_input_size;  // <- fill out

    if (has_block) {
        m_input.pop(m_model_block.data(), k_model.m_input_size);         // <- fill out
        m_engine.process(m_model_block.data(), k_model.m_input_size, m_state);
        m_output.push(m_model_block.data(), k_model.m_input_size);       // <- fill out
    }
}
```

Note:
- The other half of step 3's `process_block`, the same lines
- `WorkerThread` calls it over and over: no block yet, it checks again right away, no sleep. Be careful, this is not the best way to do it, because it makes your CPU run hot! 
- That keeps one core busy; part 2 does it better
- A whole block: take it out, run the model, push it into `m_output`
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
- So we keep the same stream as step 2, but the engine is off the audio thread
- Audio thread is real-time safe, the model runs beside it
- The host hears it exactly the prefill late
- So how do we line up a dry signal with it?
