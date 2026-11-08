<h1>Step 9<br>The model with anira</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- Back to coding.
- Step 9: the model from part 1, now on anira.
- Same model, same check.
- But steps 3, 6 and 7 are not our code any more.

---

<div class="tag">Motivation</div>

## Six things, by hand in part 1.<br><span class="then">Now declared, or handled by anira.</span>

<div class="needs">
  <div class="card need">{{ICON:model}}<div class="card-title">Inference</div><div class="card-text">steps 1–3 · engine, block, state by hand</div><div class="need-how decl">declared · <code>model.json</code></div></div>
  <div class="card need">{{ICON:gauge}}<div class="card-title">Benchmarking</div><div class="card-text">step 4 · one run, on our machine</div><div class="need-how decl">declared · <code>contract.json</code>: <code>"measured"</code></div></div>
  <div class="card need">{{ICON:shield}}<div class="card-title">Real-time safety</div><div class="card-text">step 5 · RTSan, run once</div><div class="need-how auto">handled by anira · nonblocking audio-thread calls</div></div>
  <div class="card need">{{ICON:threads}}<div class="card-title">Threading</div><div class="card-text">steps 6, 8 · a worker per instance</div><div class="need-how auto">handled by anira · one pool per process</div></div>
  <div class="card need">{{ICON:clock}}<div class="card-title">Minimal latency</div><div class="card-text">step 7 · a prefill, counted by hand</div><div class="need-how auto">handled by anira · you read <code>get_latency()</code></div></div>
  <div class="card need">{{ICON:flag}}<div class="card-title">Real-time guarantees</div><div class="card-text">steps 4, 7 · hoping the budget holds</div><div class="need-how decl">declared · <code>contract.json</code>: <code>on_miss</code></div></div>
</div>

Note:
- The six things from part 1, now that we know how anira works.
- Inference, steps 1 to 3: engine, block size and state, by hand.
- Now the model file says it.
- Benchmarking, step 4: one run, on our machine.
- Now the contract says "measured", and prepare does it.
- Real-time safety, step 5: RTSan, once.
- Now the audio-thread calls are anira's: nonblocking, and checked.
- Threading, steps 6 and 8: a worker per instance.
- Now one pool, with its defaults.
- Minimal latency, step 7: a prefill, counted by hand.
- Now prepare computes it.
- We only read it.
- Real-time guarantees: we hoped the budget held.
- Now the contract says what a late block delivers.
- Three of them we declare, in two files.
- The other three anira handles.
- We write nothing for them.
- So what is left to write?
- Next slide.

---

<div class="tag">Goal</div>

## Three configs, three calls.<br><span class="then">Same output, same check.</span>

<div class="cards">
  <div class="card">{{ICON:model}}<div class="card-title">what</div><div class="card-text">the engine and the file, the tensors, their roles, the state pair</div><div class="need-how decl">you fill · <code>model.json</code></div></div>
  <div class="card-arrow">+</div>
  <div class="card">{{ICON:sliders}}<div class="card-title">how</div><div class="card-text">budget, warm-up, on a miss</div><div class="need-how auto">given · <code>contract.json</code></div></div>
  <div class="card-arrow">+</div>
  <div class="card">{{ICON:chip}}<div class="card-title">with what</div><div class="card-text">the pool: threads, how they wait</div><div class="need-how auto">given · <code>ContextConfig{}</code>, defaults</div></div>
</div>

<div class="cards">
  <div class="card">{{ICON:code}}<div class="card-title">create</div><div class="card-text">pipeline and handler, in the constructor</div><div class="need-how auto">given</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:clock}}<div class="card-title">prepare</div><div class="card-text">host blocks in, latency out</div><div class="need-how decl">you fill · 2 blanks</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:wave}}<div class="card-title">process</div><div class="card-text">one call per callback: live, or offline waiting</div><div class="need-how decl">you fill · 2 blanks</div></div>
</div>

<div class="cards-note">The calls are in <code>AniraProcessor.cpp</code>.</div>

Note:
- Three configs and three calls.
- The goal: the same output and the same check as in part 1.
- The model file is the what.
- Which engine, which file: ONNX Runtime here.
- Which tensors, which role each plays.
- Which output feeds the state.
- That one you fill in.
- The contract is the how.
- The budget, the warm-up, what a miss delivers.
- Given, three lines.
- The context is the with what.
- The pool: how many threads, how they wait.
- Given, in AniraProcessor.h: ContextConfig with its defaults.
- anira could read it from a file too.
- We do not need one.
- Then the three calls, in AniraProcessor.cpp.
- Create is given: the pipeline and the handler, in the constructor.
- Prepare: you give the contract the host's blocks and read the latency back.
- Process: one call per callback, in place.
- The check is step 7's: the output against the reference, shifted by the latency anira reports.

---

<div class="tag">Given</div>

## 1 new library, 2 files to edit.

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
    <div class="given-folder">09_anira/exercise/ · CMake</div>
    {{FILE:contract.json|new}}
    {{FILE:anira|new}}
    {{FILE:host.h}}
    {{FILE:support.h}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">09_anira/exercise/</div>
    {{FILE:model.json|new}}
    {{FILE:AniraProcessor.cpp}}
  </div>
</div>

Note:
- CMake fetches anira, pinned to a v3 pre-release, with ONNX Runtime built in.
- The contract is complete.
- The model file has gaps.
- The tensors are named.
- Their shapes and the state pair are your task.
- Your other file is AniraProcessor.cpp.

---

<!-- .slide: class="dense-code" -->

<div class="tag">Helpers: <code>model.json</code></div>

## What the model is.<br><span class="then">Every tensor, its shape, its role.</span>

<div class="mj">

```json
{
  "models":  [ { "engine": "onnxruntime", "path": "…/model_forward.onnx" } ],
  "inputs":  [ { "name": "audio",     "role": "streamed",
                 "axes": [["batch",1],["channel",1],["time",?]] },
               { "name": "state_in",  "role": "state",
                 "axes": [["batch",1],["any",?]], "state_source": "?" } ],
  "outputs": [ { "name": "audio_out", "role": "streamed",
                 "axes": [["batch",1],["channel",1],["time",?]], "latency": ? },
               { "name": "state_out", "role": "state", "axes": [["batch",1],["any",?]] } ]
}
```

<div class="mj-keys">
  <div><b><code>models</code></b>which engine, which file</div>
  <div><b><code>axes</code></b>the shape, with names; <code>time</code>: samples per inference</div>
  <div><b><code>role</code></b><code>streamed</code>: cut into model blocks · <code>state</code>: carried from call to call</div>
  <div><b><code>state_source</code></b>the output that feeds a state input</div>
  <div><b><code>latency</code></b>the model's own delay, on its output</div>
</div>

</div>
Note:
- The model file as you get it.
- Every key is in place.
- Every blank is a question mark.
- The comment next to each blank says TODO 1 or TODO 2.
- Only the dtype is left out on the slide: float32 is the default.
- On the right, the five keys you need to read it.
- models: the engine, ONNX Runtime, and the path to our ONNX file.
- inputs: the model's input tensors.
- The first is audio.
- Its role is streamed: audio comes in continuously, and anira cuts it into blocks for the model.
- Its axes are its shape, each with a name.
- Batch 1, channel 1, time 2048.
- A tensor of 1 by 1 by 2048.
- The time axis says how many samples one inference takes.
- 2048, our input size.
- The second input is state_in.
- Role state: no time axis.
- It is carried from one call to the next.
- The outputs mirror the inputs.
- audio_out, streamed, the same shape.
- state_out, the new state.
- TODO 1 is the audio: the time size, in and out.
- And on audio_out the model's own latency.
- You know that number from step 7.
- TODO 2 is the state: its size, and which output feeds it.
- Next slide.

---

<div class="tag">Helpers: the state pair</div>

## The state loop, declared.<br><span class="then">anira carries it, no copy.</span>

<div class="state-demo">
  <div class="state-row was">
    <div class="state-label">step 2</div>
    <code>std::copy_n(m_state_out.begin(), m_state_out.size(), state.begin());</code>
  </div>
  <div class="state-row is">
    <div class="state-label">step 9</div>
    <code>{ "name": "state_in", "role": "state", "axes": [["batch",1],["any", ?]], "state_source": ? }</code>
  </div>
</div>

<div class="statement-points">
  <div>The size: as many floats as the state of step 2</div>
  <div><code>state_source</code>: the output that feeds <code>state_in</code>. <b>A</b> <code>"audio"</code> · <b>B</b> <code>"audio_out"</code> · <b>C</b> <code>"state_out"</code>?</div>
</div>

Note:
- Remember the state loop from step 2.
- Every block, we copied the state that came out back into the state that goes in.
- By hand.
- In anira the state is two tensors with the role state.
- state_in goes into the model, state_out comes out.
- You declare their size: as many floats as in step 2.
- And which output feeds the input: state_source.
- Then anira carries it.
- Two buffers, swapped after every inference.
- No copy, no code on your side.
- So which output is it?
- A, audio.
- B, audio_out.
- C, state_out.
- Try one.
- A wrong one is refused at create, and the message says why.
- The pair tells anira one more thing: every call needs the state of the one before.
- So it never runs two blocks of this model at once, and keeps them in order.

---

<div class="tag">Helpers: <code>contract.json</code></div>

## How it runs.

```json
{ "hard": { "budget": "measured", "warmup": "until_stable", "on_miss": "bypass" } }
```

Three lines. The geometry comes from the host, at `prepare`.

<div class="note-box">The third config, the context: nothing to write. One pool per process, with its defaults.</div>

Note:
- The contract is three lines.
- Hard, because this is a real-time stream.
- The host calls with fixed blocks, every block has a deadline, nothing may wait.
- Jobs without a stream, images for example, get an async contract in a later release.
- The budget is measured.
- anira benchmarks the model at prepare, on this machine, with this engine.
- That is step 4, done by the library.
- The warm-up runs until the times are stable, before prepare returns.
- We saw why in step 4.
- ONNX Runtime's first call took 18 ms instead of 13.
- LibTorch's first two, 80 and 38 ms instead of 7.7.
- Those calls now happen before the audio starts.
- On a miss: bypass.
- A block whose result is late goes out as the input block.
- Not as zeros, as in step 7.
- The host's blocks come in at prepare.
- The third config, the context: we write nothing.
- anira takes its defaults, one pool per process.

---

<div class="tag">Task: <code>model.json</code> · <code>AniraProcessor.cpp</code></div>

## Task

<div class="task-timer" data-minutes="10"></div>

Fill in the TODOs in `steps/09_anira/exercise/model.json` and `AniraProcessor.cpp`

```bash
# validate: the check against the reference
cmake --build --preset release --target step09_exercise_check
./build/release/bin/step09_exercise_check
# play: the same processor as a plugin (or step09_exercise_VST3)
cmake --build --preset release --target step09_exercise_Standalone
```

<div class="run">
  <div class="run-step" data-seconds="2.4">
    <div class="run-title"><span>1</span>Declare the tensors</div>
    <div class="run-where">in <code>model.json</code></div>
    <div class="run-visual run-setup">
      <div class="run-note">time · time · latency</div>
    </div>
  </div>
  <div class="run-step" data-seconds="2.4">
    <div class="run-title"><span>2</span>Declare the state</div>
    <div class="run-where">in <code>model.json</code></div>
    <div class="run-visual run-setup">
      <div class="run-note">size · size · <code>state_source</code></div>
    </div>
  </div>
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>3</span>Prepare and process</div>
    <div class="run-where">in <code>AniraProcessor.cpp</code></div>
    <div class="run-visual run-worker">
      <div class="q small one"><div class="q-slot"><b></b></div></div>
      <div class="mini-model">anira</div>
      <div class="q small one out"><div class="q-slot"><b></b></div></div>
      <div class="run-ok">✓ matches the reference, shifted by <code>get_latency()</code></div>
    </div>
  </div>
</div>

Note:
- Ten minutes.
- TODO 1 and 2 are in model.json: replace every question mark with its value.
- A number as it is, 2048, no quotes.
- A name in its quotes.
- The comment next to each blank says where the value comes from.
- TODO 3 is in AniraProcessor.cpp: four blanks, marked "fill out".
- Then build step09_exercise_check and run it.
- It names what is still missing, with the line.
- You are done when it matches the reference.
- Then the same processor runs as a plugin: step09_exercise_Standalone.

---

<div class="tag">Solution</div>

## TODO 1 · Declare the tensors

```json
"inputs": [
  { "name": "audio", "role": "streamed",
    "axes": [["batch", 1], ["channel", 1], ["time", «2048»]] },   // <- solution
  …
],
"outputs": [
  { "name": "audio_out", "role": "streamed",
    "axes": [["batch", 1], ["channel", 1], ["time", «2048»]],     // <- solution
    "latency": «2224» },                                          // <- solution
  …
]
```

Note:
- TODO 1, the audio.
- Audio in and out: a time axis of 2048.
- One model block, as in step 1.
- On audio_out: the model's own latency, 2224, as in step 7.
- A wrong size is refused at prepare.
- anira names the tensor and both shapes.
- A wrong latency is not refused: the file cannot know it.
- The check finds it, because the output lines up at another shift.
---

<div class="tag">Solution</div>

## TODO 2 · Declare the state

```json
{ "name": "state_in",  "role": "state",
  "axes": [["batch", 1], ["any", «63488»]],     // <- solution
  "state_source": "«state_out»" },              // <- solution: C
…
{ "name": "state_out", "role": "state",
  "axes": [["batch", 1], ["any", «63488»]] }    // <- solution
```

Note:
- The state: 63,488 floats in and out, the size from step 2.
- The pairing is one field, state_source.
- C, state_out.
- A: audio is no output.
- B: audio_out is streamed audio.
- Create refuses both, and says why.
- Leave it out, and create refuses that too: a state input needs a source.
- With the pair in place, anira knows each call depends on the one before.
- So it never runs two blocks of this model at once, and it keeps them in order.

---

<div class="tag">Solution</div>

## TODO 3 · Prepare and process

```cpp
// prepare()
m_contract.hard_host_blocks(«1», «max_block_size», «sample_rate»);  // <- 1. solution
const anira::PlanReport report = m_handler->prepare(m_contract);  // warm-up runs here
m_budget_ms = report.plans().front().measured_ms;
m_latency = «m_handler->get_latency(0)»;  // <- 2. solution: output slot 0

// process()
if (offline) {   // <- 3. solution: a bounce, wait for the result
    status = m_handler->process_wait(«block», 0, «block», 0, ANIRA_WAIT_FOREVER);
} else {         // <- 4. solution: live, never waits
    status = m_handler->process(«block», 0, «block», 0);
}
```

Note:
- Given: the constructor reads the model file, drops its comments, builds the pipeline and the handler.
- The prepare call is given too.
- The warm-up and the measurement run inside it.
- Blank 1: the host's side of the contract.
- Any block from 1 up to its largest, and the sample rate.
- Blank 2: the latency of output 0.
- On my machine 5295: a budget of about 12.7 ms, for any host block up to 512.
- Yours depends on your machine.
- Blank 3, offline: a bounce.
- process_wait with ANIRA_WAIT_FOREVER waits for every result.
- Nothing is missed.
- Blank 4, live: in the callback, process over the block, in place.
- It never waits.
- The same four values in both calls.
- The check runs offline first, so the comparison is exact.
- Then once live.

---

<div class="tag">anira's report</div>

## What prepare measured.<br><span class="then">On this machine, before the first block.</span>

<div class="report">
  <div class="report-card">
    <div class="report-head"><strong>model</strong><span class="report-pill ok">comfortable</span></div>
    <div class="report-times"><span>budget <b>12.9 ms</b></span><span>measured <b>12.9 ms</b></span><span>median <b>12.8 ms</b></span></div>
    <div class="report-line">warm-up: 4 inferences, until stable</div>
    <div class="report-line">load <b>28 %</b>: real-time factor 0.28 over 1 inference</div>
    <div class="report-bar"><i style="width:22.4%"></i><b style="left:60%"><span>75 %</span></b><b style="left:80%"><span>100 %</span></b></div>
  </div>
  <div class="report-scale">
    <div><span class="report-pill ok">comfortable</span>below 75 %</div>
    <div><span class="report-pill marginal">marginal</span>below 100 %, may drop out under load</div>
    <div><span class="report-pill over">not real-time</span>held back live, until <strong>Run anyway</strong></div>
  </div>
</div>

<div class="cards-note">The check prints it, the plugin shows it under <strong>report</strong>: <code>handler.plan_report()</code></div>

Note:
- prepare does not only plan.
- It warms the model up and measures it, on this machine, with this engine.
- Step 4, done by the library.
- One row per plan.
- The budget anira plans with.
- What it measured: the maximum of the last warm-up inferences.
- And the median, the jitter at a glance.
- The load: the inference time over the audio it produces.
- Here 28 %.
- The model needs about a quarter of the time it has.
- "Over 1 inference": the model carries its state, so its inferences run one after another.
- A stateless model could run several at once, and the load would be shared.
- Below 75 % comfortable.
- Below 100 % marginal.
- Above, not real-time.
- Not real-time: live, the plugin holds back.
- The window says so.
- "Run anyway" plays it nonetheless, with dropouts and clicks.
- A bounce always processes: it waits for every result.
- Your check prints your report.
- The plugin's report button shows it.

---

<div class="tag">Done</div>

## Steps 3, 6 and 7,<br><span class="then">in two files.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
- That is steps 3, 6 and 7, in two files.
- No ring buffer, no worker, no queue, no state copy, no latency arithmetic.
- And step 7's check passes.
- Next: we take the model apart.
