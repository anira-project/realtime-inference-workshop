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
- The check runs offline and makes sure our output matches the Python reference.
- And that the reported latency is calculated right.

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
  "inputs":  [ { "name": "audio_in",  "role": "streamed",
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
- Basically, there are two TODOs here.
- TODO 1 defines the size of the input and output audio tensors, and the latency on audio_out.
- TODO 2 defines the state size, and gives state_in its state_source.
- Think about which output tensor has to go there, by name, to keep the inference stateful.
- All the numbers come from part 1: steps 1, 2 and 7.
- If you pick a wrong state_source, create refuses it and says why.
- That is the model file, the what: now the how.

---

<div class="tag">Helpers: <code>contract.json</code></div>

## How it runs.

```json
{ "hard": { "budget": "measured", "warmup": "until_stable", "on_miss": "bypass" } }
```

Three keys. The host's blocks come in at `prepare`.

Note:
- The second file, contract.json, is already filled in for you.
- Hard means a stream: every callback has a deadline.
- Budget measured: we no longer benchmark by hand.
- anira benchmarks the model inside our product, on the user's machine.
- That is handy once you ship it: every machine gets its own number.
- Warm-up until stable: the slow first runs happen before the audio starts.
- On a miss, bypass: the dry input goes out instead of silence.

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
- Okay, now we've reached the step 9 task: fill out the TODOs.
- There are three of them.
- TODO 1: declare the audio tensors.
- TODO 2: declare the state, so it is fed back after every inference.
- TODO 3: pass everything prepare and process need.
- Before you start the plugin, build and run the exercise check first.
- It validates that all your blanks are filled in correctly.
- Once it is green, play with the plugin; the standalone is the easiest.
- The model still runs at 44.1 kHz: at 48 the plugin asks first, and it won't sound accurate.
- But now any buffer size from the host works.
- [start the timer] Ten minutes; we walk around, wave if you get stuck.

---

<div class="tag">Solution</div>

## TODO 1 · Declare the tensors

```json
"inputs": [
  { "name": "audio_in", "role": "streamed",
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
- TODO 1: 2048 in and out, one model block, as in step 1.
- On audio_out, the model's own latency: 2224 samples, as in step 7.
- A wrong size is refused at prepare, when anira opens the ONNX file.
- [pause] A wrong latency is not refused: the ONNX file has no delay to check against.
- But the check catches it: the output no longer lines up with the reference.
---

<div class="tag">Solution</div>

## TODO 2 · Declare the state

```json
{ "name": "state_in",  "role": "state",
  "axes": [["batch", 1], ["any", «63488»]],     // <- solution
  "state_source": "«state_out»" },              // <- solution
…
{ "name": "state_out", "role": "state",
  "axes": [["batch", 1], ["any", «63488»]] }    // <- solution
```

Note:
- TODO 2: step 2's state size, in and out, and the state_source.
- [ask the room, hands up] Who tried a wrong one first, and read the message?
- The answer is state_out.
- audio_in is no output at all, and audio_out is streamed audio, not state.
- Create refuses both, and says why.
- With the pair in place, anira runs the inferences one after another: each needs the state of the one before.

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
- TODO 3: where our plugin talks to anira.
- Blank 1, the host's blocks: why 1 and not 512? [pause]
- Any block size may come, down to a single sample, and the check sends exactly that.
- Blank 2: the latency of output slot 0, audio_out.
- On this machine that is 5295 samples [say: fifty-two ninety-five]; yours depends on your machine.
- Blank 3, offline, a bounce: process_wait waits for every result.
- Blank 4, live, in the callback: process never waits.

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
- [before: step09_exercise_Standalone built, interface at 48 kHz]
- This is what prepare measured on this machine, before the first block.
- Budget and measured are the same: the slowest of the last four warm-up runs.
- The warm-up stopped after four runs, because they already agreed.
- A load of 28 percent: comfortable.
- A model that is not real-time is held back, until you press Try nonetheless.
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
- In two files, we replaced steps 3, 6 and 7.
- Gone: the ring buffer, the worker, the latency arithmetic, and step 2's state copy.
- And step 7's check still passes, with anira's latency instead of ours.
- Next, step 10: we cut the model in the middle.
