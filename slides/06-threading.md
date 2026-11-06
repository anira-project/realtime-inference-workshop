<h1>Step 6<br>Inference on a<br>worker thread</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- The forward pass cannot stay on the audio thread for two reasons:
- 1. It's not fast enough
- 2. It's not safe
- So it gets its own thread: the worker thread

**Script:**

"In step 5 we found out that the engine can't run on the audio thread. The solution was to move the inference to a separate thread. In this step we'll give it its own thread, the worker thread."

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
  <div><strong>Real-time capable</strong><br>The worker has its own budget: 46.4ms (step 4)</div>
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
- Real-time capable, step 4: even a typical call of 13 ms is longer than a our host callback; the worker isn't inside any callback, it gets its own time
- Real-time safe, step 5: the engine's allocations and locks leave the audio thread


**Script:**

"Here's the idea. We now have two threads. The audio thread keeps its pace: in every callback it only moves samples around, nothing else. It pushes the host's samples into the input ring buffer, `m_input` as in step 3, but now a thread-safe one. Once there are 2048 samples in it, the worker takes them out, runs the model and pushes the result into `m_output`, where the audio thread picks it up.
The worker is allowed to allocate. It doesn't have a per-callback deadline. It does have a budget: the rounded max inference time you measured in step 4. The audio thread never waits for it.

*Click: the two problems.*

So with the worker thread we solve two problems at once. First, real-time safety from step 5: the engine's allocations and locks are no longer on the audio thread. And second, real-time capability from step 4: even a typical call of 13 milliseconds is longer than a callback. Now the forward pass no longer runs inside any callback, it gets its own time."

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

**Script:**

"So our goal is to split our processor from step 3 over two threads, with a latency that stays fixed. First, in `prepare`, we give the ring buffers their sizes. Starting the thread is given. Then we write the audio thread's side: samples into `m_input`, samples out of `m_output`, and it never waits. And then the worker's side: take a block out of `m_input`, run the model, push it into `m_output`."

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

**Script:**

"We still have the model, the signals and the engine from the steps before. New is the `ThreadSafeRingBuffer`: it replaces the ring buffer from step 3, with the same functions, but safe between two threads. Underneath is a lock-free buffer from moodycamel, which CMake fetches for us. Our host loop from step 3 gets one line more: after every callback it sleeps for as long as one block of audio lasts, 11.6 milliseconds. With two threads, this only works if the audio thread is called at the audio rate, and the sleep simulates the audio device. `host.h` gives us that duration. The `WorkerThread` takes care of the thread: it calls one function over and over, from `start` until `stop`."

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


**Script:**

"And this is our host. It's a DAW without the DAW: the loop in `main` from step 3, that calls our callback block by block, just like an audio callback, so we overwrite the samples in place.

New is one line: after every callback the loop sleeps for as long as one block of audio lasts, 11.6 milliseconds. `block_duration` from `host.h` works that out. Without it, the loop would call the audio thread as fast as it can, and the worker could never keep up. With the sleep, it behaves like a real audio device.

The block size is just a parameter, and for the whole first part it is 512. The check at the end looks at whether the host gets the model's stream back, exactly the prefill later."

---

<div class="tag">Helpers: <code>threadsafe_ring_buffer.h</code></div>

## One writer, one reader<br><span class="then">No locks.</span>

<table class="budget-table rb-table">
  <thead><tr><th></th><th>Audio thread <small>one sample, never waits</small></th><th>Worker <small>whole blocks</small></th></tr></thead>
  <tbody>
    <tr><td><code>m_input</code></td><td><code>try_push(sample, num_samples)</code> <small>false if full</small></td><td> <code>pop(block, block_size)</code> & <code>available()</code></td></tr>
    <tr><td><code>m_output</code></td><td><code>try_pop(sample, num_samples)</code> <small>false if empty</small></td><td><code>push(block, block_size)</code></td></tr>
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
- Worker: `available`, `pop`, `push` for whole blocks; `pop` takes nothing and returns false if too few are there, like in step 3
- Underneath: moodycamel's circular buffer, only its `try_` functions: never wait, never allocate, never throw
- `WorkerThread`: given, calls our `process_model` over and over, from `start` until `stop`

**Script:**

"Let's look at the helpers. Read this table by row: each ring buffer has exactly one thread writing and one thread reading. The audio thread writes into `m_input` and the worker reads from it. For `m_output` it's the other way round.

Why can't we just use step 3's ring buffer? Because there, `push` and `pop` both change the same counter, `m_size`. If two threads do that at the same time, an update can get lost, and the reader could even see the new count before the sample is written. The thread-safe version avoids that: the writer only moves the write position, the reader only moves the read position, and nothing is written by both. So there's nothing to lock. That's the simplest lock-free case there is.

The audio thread gets `try_push` and `try_pop`: one sample at a time, and a bool says whether it worked. The worker gets step 3's functions for whole blocks: `available`, `pop` and `push`. Underneath is a circular buffer from moodycamel, and we only use its try functions, which never wait, never allocate and never throw.

And the thread itself is given: the `WorkerThread` calls our `process_model` over and over, from `start` until `stop`. Nothing here waits: on the audio thread, waiting would be a missed deadline for the whole host."
  

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

**Script:**

"Now it's your turn. First, if you haven't yet, put your number from step 4 into `k_max_inference_ms` in `steps/common/inference_time.h`. Then there are three TODOs: the ring buffer sizes in `prepare`, the audio thread, and the worker thread. You'll find the `main.cpp` in `steps/06_threading/exercise`, and the build commands are on the slide. This is the biggest task so far, but it's one line at a time, so you get 10 minutes. When it works, the host hears the model's stream, exactly the prefill late. And as always, the solution is in the `solution` folder."

*Timebox.*

---

<div class="tag">Solution</div>

## TODO 1: The ring buffer sizes

```cpp
// given: how much silence m_output starts with
adaptation = (k_model.m_input_size - max_block_size);
m_prefill = adaptation + k_model.rounded_max_inference;

const size_t input_capacity = k_model.m_input_size;  // <- fill out
const size_t output_capacity =
    k_model.rounded_max_inference + k_model.m_input_size;  // <- fill out

// given: the silence goes in, and the worker thread starts
m_output.push(silence.data(), m_prefill);
m_worker.start([this] { process_model(); });
```

Note:
- `m_input`: one model block, as in step 3: the worker takes it out as soon as it's full
- `m_output`: at least one model block, so the processed samples fit + the rounded_max_inference_samples.
- The worst case is in the first block, were w haven't passed any samples yet, so the output RB is empty.
- Given: the prefill, the silence `m_output` starts with, so the host never finds it empty. Two waits:
  - `adaptation`: 2048 − 512 = 1536. Callbacks 1–3, while the first block fills up, as in step 3
  - the worker's time: then the block still has to go through the model. Your worst case from step 4, rounded up to whole callbacks
- Together 2560: that's how late the host hears the stream
- `m_output`: the worker's time plus one model block. Fullest moment: a fast worker pushes a new block while the worker's time is still waiting in there

**Script:**

"The input ring buffer holds one model block, like in step 3: as soon as it's full, the worker takes it out.

For the output, let's first look at the prefill: the silence it starts with, so the host never finds it empty. It covers two waits. First, the first block has to fill up: callbacks 1 to 3, that's 2048 minus 512, 1536 samples, like in step 3. Then that block still has to go through the model, and that takes the worker's time: your worst case from step 4, rounded up to whole callbacks. Together that's 2560 samples, and that's how late the host hears the stream.

And that explains the output's size. When the worker is fast, its new block arrives while the worker's time of prefill is still waiting in the buffer. So it needs room for the worker's time plus one model block."

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

**Script:**

"The audio thread is step 3's `process_block`, without the engine, and one sample at a time. You only had to fill in the `try_push` and the `try_pop`. Neither of them waits, they just tell us whether it worked. If a sample doesn't fit into the input, we count the rest and stop. If a sample isn't in the output yet, the rest of the callback goes out as zeros. The code around that, the zeros and the debt, is given, and the next slide shows why it's there.

Notice what's not in here: no `new`, no mutex, no engine, and no waiting."

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
- This works like a safety net, so we are never late
- It's not perfect, though. The way we handle it now will create short term shift, because we don't 

**Script:**

"The prefill gives the worker its time. But what if it's late anyway, because one call was slow?

*Top row.* On time, the samples arrive in order.

*Click.* Now samples 3 and 4 aren't there yet. The audio thread doesn't wait, so the host gets silence. That's a dropout. If we just pop them later, everything after them is two samples late, and our latency has grown.

*Click.* So we keep a debt: we count the zeros we sent, and throw 3 and 4 away when they arrive. Then 5 lands where it belongs. A dropout, but no shift.

The same can happen on the way in, counted in `m_unpushed`. It's a safety net, and it's given in TODO 2."

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
- `WorkerThread` calls it over and over: no block yet, it checks again right away, no sleep. Be careful, this is not the best way to do it, because it will make you cpu run hot! 
- That keeps one core busy; part 2 does it better
- A whole block: take it out, run the model, push it into `m_output`
- The engine may allocate and lock here: nobody waits on this thread

**Script:**

"The worker side is the other half of step 3's process_block, with the same lines. The `WorkerThread` calls it over and over. If there's no whole block in the input ring buffer yet, it simply checks again right away, without sleeping, because the result is needed soon. That keeps one core busy, and in part 2 we'll see how to do it better. If there is a block, we take it out, run the model and push the result into the output ring buffer. And here the engine may allocate and lock as much as it wants: nobody is waiting on this thread."

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

**Script:**

"The result: the same stream as in step 2, and the engine is off the audio thread. The audio thread is now real-time safe, and the model runs beside it."
*If possible, run it next to step 5 live.*
"And the host hears it exactly the prefill late. But the model itself answers late too. So what's the one number we tell the host, and how do we line up a dry signal with it? That's our next step: latency."
