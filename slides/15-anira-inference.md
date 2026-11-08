<h1>Step 9<br>The model with anira</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- Thank you, Fares, for the overview.
- Let's go back to coding.
- In step 9 we redo part 1, now with anira.
- Same model, same test signal, same check.

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
- Here are the six things again that we built by hand in part 1.
- Inference: which model, which engine, which tensors, now all defined in model.json.
- Benchmarking: defined in our contract.json.
- Real-time safety and threading are built into anira: one thread pool, from the anira context.
- Latency: anira calculates it, and we only read it.
- Real-time guarantees: what happens on a missed deadline is also in the contract.
- [pause] So what is left for us to write?

---

<div class="tag">Goal</div>

## Three configs, three calls.<br><span class="then">Same output, same check.</span>

<div class="cards rows-top">
  <div class="card">{{ICON:model}}<div class="card-title">what</div><div class="card-text">the engine and the file, the tensors, their roles, the state pair</div><div class="need-how decl">you fill · <code>model.json</code></div></div>
  <div class="card-arrow">+</div>
  <div class="card">{{ICON:sliders}}<div class="card-title">how</div><div class="card-text">budget, warm-up, on a miss</div><div class="need-how auto">given · <code>contract.json</code></div></div>
  <div class="card-arrow">+</div>
  <div class="card">{{ICON:chip}}<div class="card-title">with what</div><div class="card-text">the pool: threads, how they wait</div><div class="need-how auto">given · <code>ContextConfig{}</code>, defaults</div></div>
</div>

<div class="cards rows-bottom">
  <div class="card">{{ICON:code}}<div class="card-title">create</div><div class="card-text">pipeline and handler, in the constructor</div><div class="need-how auto">given</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:clock}}<div class="card-title">prepare</div><div class="card-text">host blocks in, latency out</div><div class="need-how decl">you fill · 2 blanks</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:wave}}<div class="card-title">process</div><div class="card-text">one call per callback: live, or offline waiting</div><div class="need-how decl">you fill · 2 blanks</div></div>
</div>

<div class="cards-note rows-bottom">The calls are in <code>AniraProcessor.cpp</code>.</div>

<span class="fragment rows-focus"></span>

Note:
- What is left: three configs on top, three calls below.
- First, the three configs.
- model.json has gaps for you: the tensor sizes, and how to handle the state.
- contract.json defines how the model runs; it is complete, and we look into it in a second.
- And the context: we use its defaults, so the thread pool is half the number of logical cores.
- [click] Then the three calls: create, prepare and process.
- Create is already done for you, in the constructor.
- In prepare you fill two blanks: tell the contract the host's blocks, and read back the latency.
- In process you fill two more: one call per callback, live or offline.
- The check runs process offline and waits for each result.
- It removes the latency anira adds, so the output lines up with the reference.

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
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">09_anira/exercise/</div>
    {{FILE:model.json|new}}
    {{FILE:AniraProcessor.cpp}}
  </div>
</div>

Note:
- So what are the provided files?
- Same assets as in part 1: the model, the test signal and the reference.
- What helpers are new?
- We provide anira itself, bundled and included automatically.
- And the contract file, already filled in.
- You edit two files: the gaps in model.json, and four blanks in AniraProcessor.cpp.

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
- This is the model file Fares showed you, now with question marks.
- The comment next to each blank says which TODO it is.
- [pause] TODO 1, the audio: the time size in and out, and the latency on audio_out.
- TODO 2, the state: its size, and which output feeds it.
- All the numbers come from part 1: steps 1, 2 and 7.
- The state pair, on the next slide.

---

<div class="tag">Helpers: the state pair</div>

## The state loop, declared.<br><span class="then">anira carries it, no copy.</span>

<div class="state-demo">
  <div class="state-row was">
    <div class="state-label">step 2</div>
    <code>std::copy_n(m_outputs[1].GetTensorMutableData&lt;float&gt;(), state_size, state);</code>
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
- Remember the state loop from step 2, the line at the top.
- Every block the new state was copied back over the old one, in our vector.
- Below it, step 9: the same loop, declared in one line.
- Size and source declared, anira carries it: two buffers, swapped after every run.
- So which output feeds state_in: A, B, or C?
- [pause, leave it open] You saw the answer in the create chapter, so try a wrong one first and read what create says.
- That is the model file, the what: now the how.

---

<div class="tag">Helpers: <code>contract.json</code></div>

## How it runs.

```json
{ "hard": { "budget": "measured", "warmup": "until_stable", "on_miss": "bypass" } }
```

Three keys. The host's blocks come in at `prepare`.

<div class="note-box">The third config, the context: nothing to write. One pool per process, with its defaults.</div>

Note:
- Hard: the stream from the prepare chapter, every callback with a deadline.
- Budget measured: prepare benchmarks it on this machine, and we read its report after the task.
- Warm-up until stable: remember Lina's benchmark, the first pass 18 ms, then 13.
- Those slow passes now run inside prepare, before the audio starts.
- On a miss, bypass: where step 7's late callback left the wet path as zeros, the input block goes out.
- No wait_ratio line: its default is 0, the callback never waits for a result.
- [pause] No block sizes here: that is your blank, in prepare.
- That is both files: now fill in the gaps.

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
      <div class="run-ok">✓ matches the reference</div>
    </div>
  </div>
</div>

Note:
- Ten minutes, three TODOs, two files.
- TODO 1 and 2, in model.json: replace every question mark, numbers without quotes, names in quotes.
- Each comment says where its value comes from: a step of part 1, the matching tensor, or the three options.
- The state size is on step 2's slide, or in its main.cpp, the k_model constant.
- TODO 3 is in AniraProcessor.cpp: four blanks, marked "fill out".
- For the first one mind the order: smallest block, largest block, sample rate.
- The C++ file does not compile until all four blanks are filled.
- The check reports the model file's blanks, with the line number.
- It also prints anira's plan report: we read that together afterwards.
- Don't worry if you don't finish: we solve it together, the solution sits next to the exercise.
- [start the timer] We walk around: wave if the check refuses something you don't understand.

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
- TODO 1, the audio: 2048 in and out, one model block, step 1's fixed size.
- On audio_out, the model's own latency: 2224 samples, as in step 7.
- The same wrong size in and out is refused at prepare, not at create.
- Prepare is when anira first opens the ONNX file: create reads only your declaration.
- The refusal names the tensor and both shapes, the model's and yours.
- Two different sizes create refuses already: one streamed block, one hop.
- [pause] A wrong latency, though, is not refused: the ONNX file carries no delay figure to check against.
- The check finds it: your latency moves anira's number, not the audio, so the output lines up at another shift.
- And its message names both numbers, yours and the model's own.
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
- TODO 2, the state: step 2's number in and out, and the pairing, one field, state_source.
- [ask the room, hands up] Who tried a wrong one, A or B, and read the message?
- The answer is C, state_out.
- A, audio, is no output at all, and B, audio_out, is streamed audio, not state.
- Create refuses both, and says why, by name.
- Leave it out, and create refuses that too: a state input needs a source.
- With the pair in place, anira runs the inferences one after another: each needs the state before it.

---

<div class="tag">Solution</div>

## TODO 3 · Prepare and process

```cpp
// prepare()
m_contract.hard_host_blocks(«1», «max_block_size», «sample_rate»);  // <- 1. solution
const anira::PlanReport report = m_handler->prepare(m_contract);  // warm-up runs here
m_budget_ms = report.plans().front().measured_ms;
m_latency = «m_handler->get_latency(0)»;  // <- 2. solution: output slot 0 · here: 5295

// process()
if (offline) {   // <- 3. solution: a bounce, wait for the result
    status = m_handler->process_wait(«block», 0, «block», 0, ANIRA_WAIT_FOREVER);
} else {         // <- 4. solution: live, never waits
    status = m_handler->process(«block», 0, «block», 0);
}
```

Note:
- TODO 3: where the main thread and the audio thread talk to anira.
- Above this code, given: the constructor reads the model file, drops its comments, builds the pipeline and handler.
- A pipeline, the class from the architecture slide: what one handler runs, here the one model alone.
- Blank 1, the host's promise in the contract: why 1 and not 512? [pause]
- Remember the loop point from the prepare chapter: the largest block is not the only one.
- And the check itself sends blocks of 512 first, then changing sizes, one of them a single sample.
- prepare is given too, and the line below keeps the plan report's measured time.
- Blank 2: the latency of output slot 0, audio_out, the first output in the file.
- On this machine get_latency returns 5295 [say: fifty-two ninety-five], Fares's number: it covers any block up to 512.
- Yours follows your machine's budget.
- Blank 3, offline, a bounce: process_wait with ANIRA_WAIT_FOREVER waits for every result.
- Blank 4, live, in the callback: process over the block, in place.
- The check runs offline first, where nothing is missed, so the comparison is exact, then once live.

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
    <div><span class="report-pill over">not real-time</span>held back live, until <strong>Try nonetheless</strong></div>
  </div>
</div>

<div class="cards-note">The check prints it, the plugin shows it under <strong>report</strong>: <code>handler.plan_report()</code></div>
<div class="cards-note">At 48 kHz the plugin asks first: a 44.1 kHz model, no resampler. <strong>Process anyway</strong> plays it as it is.</div>

Note:
- [before: step09_exercise_Standalone built, interface at 48 kHz, as prepared at slide 1]
- prepare's plan report, one row per plan.
- Budget equals measured: the max of the last four warm-up runs.
- They differ only when the budget is given by hand, in milliseconds.
- The median sits right under the max: the times hardly spread.
- Four inferences, because the first four already agreed: within 20 percent of their median.
- Fares's slide said 12.7 ms, on his machine: either budget takes two callbacks, so the same latency.
- Over 1 inference: the state keeps the runs in order, one at a time.
- Try nonetheless plays a not-real-time model all the same, dropouts and clicks included.
- [ask the room] Your check printed yours: anyone marginal, or not real-time?
- [if time: open step09_exercise_Standalone at 48 kHz: the rate ask, Process anyway, then report]
- [in the demo] The window asks at any rate but 44.1 kHz, as step 8 did at 48: no resampler.

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
- Gone: the ring buffer, the worker, the latency arithmetic, and step 2's state copy.
- And step 7's check passes, with anira's latency instead of ours.
- Next, step 10: we cut the model in the middle.
