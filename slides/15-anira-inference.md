<h1>Step 9<br>The model with anira</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- Back to coding.
- In Step 9 we take the model from part 1 and run it through anira.
- We use the same model and the same check, but this time Steps 3, 6 and 7 are not written by hand.

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
- Back to the six things from part 1, now that we know how anira works.
- Inference, steps 1 to 3: the engine, the block size and the state, by hand. Now the model file says it.
- Benchmarking, step 4: one run on our machine. Now the contract says measured, and prepare does it.
- Real-time safety, step 5: we checked once with RTSan. Now the calls on the audio thread are anira's, nonblocking and checked.
- Threading, steps 6 and 8: a worker per instance. Now the pool, with its defaults.
- Minimal latency, step 7: a prefill we counted by hand. Now prepare computes it, and we ask for it.
- Real-time guarantees: we hoped the budget held. Now the contract says what a late block delivers.
- Three of them we declare, in the two files. The other three anira handles by itself: we write nothing for them.
- So what is left for us to write? That is the next slide.

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
- What we write is three configs and three calls, and the goal is the same output and the same check as in part 1.
- The model file is the what: the engine and the model file, ONNX Runtime here, which tensors the model has, what role each plays, and which output feeds the state. That one you fill in.
- The contract is the how: the budget, the warm-up and what a miss delivers. It is given, three lines.
- The context is the with what: the pool, how many threads and how they wait. It is given, in AniraProcessor.h: ContextConfig with its defaults. anira could read it from a file too; we do not need one.
- Then the calls in AniraProcessor.cpp. Create, the pipeline and the handler, is given; it happens in the constructor.
- In prepare you give the contract the host's blocks and read the latency back.
- In process, one call per callback, in place.
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
- CMake fetches anira, pinned to a v3 pre-release with ONNX Runtime built in.
- The contract is complete.
- The model file is given with gaps: the tensors are named, and their shapes and roles are your task.
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
                 "axes": [["batch",1],["channel",1],["time",TODO1]] },
               { "name": "state_in",  "role": "state",
                 "axes": [["batch",1],["any",TODO2]], "state_source": "TODO2" } ],
  "outputs": [ { "name": "audio_out", "role": "streamed",
                 "axes": [["batch",1],["channel",1],["time",TODO1]], "latency": TODO1 },
               { "name": "state_out", "role": "state", "axes": [["batch",1],["any",TODO2]] } ]
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
- This is the model file as you get it: every key is in place, and the blanks are marked TODO1 and TODO2. Only the dtype is left out here: float32 is the default.
- On the right, the five keys you need to read it.
- Under models: the engine, ONNX Runtime, and the path to our ONNX file.
- Under inputs: the model's input tensors. The first is audio, and it is streamed: audio comes in continuously, and anira cuts it into blocks for the model.
- Its axes are its shape, each with a name: batch 1, channel 1, time 2048, so a tensor of 1 by 1 by 2048.
- The time axis says how many samples one inference takes: 2048, our input size.
- Here we could also define a window: for a model that needs past samples, overlapping windows or a receptive field. Ours needs no lookback, so there is none.
- The second input is state_in, with the role state: no time axis, it is carried from one call to the next.
- The outputs mirror the inputs: audio_out, streamed, with the same shape, and state_out, the new state.
- TODO 1 is the model's audio: the time size of input and output, and on audio_out the model's own latency, a number you know from step 7.
- TODO 2 is the state: its size, and which output feeds it. That is the next slide.

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
- Remember the state loop from step 2: every block we copied the state that came out back into the state that goes in, by hand.
- In anira the state is two tensors with the role state: state_in goes into the model, state_out comes out.
- You declare their size, as many floats as in step 2, and which output feeds the input: state_source.
- Then anira carries it: it binds two buffers to the pair and swaps them after every inference. No copy, no code on your side.
- Which output is it? A, audio, B, audio_out, or C, state_out. Try one: if it is wrong, create says why.
- Because each call needs the state of the one before, anira never runs two blocks of this model at once, and keeps them in order.

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
- We have a hard contract because this is a real-time stream: the host calls us with fixed blocks, every block has a deadline, and nothing may wait. For jobs without a stream, like images, an async contract follows in a later release.
- The budget is measured: anira benchmarks the model at prepare, on this machine, with this engine. That is Step 4, done by the library.
- The warm-up runs until the times are stable, before prepare returns. We saw why in Step 4: the first calls took much longer, about 60 and 40 ms instead of 7.5, so we make them before the audio starts.
- And bypass on a miss: a block whose result is late goes out as the input block, not as zeros like in Step 7.
- The host's blocks come in at prepare.
- And the third config, the context: we write nothing. Without one, anira takes its defaults: one pool per process, the engines this build has.

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
- TODO 1 and 2 are in model.json: replace every TODO1 and TODO2 with its value. A number as it is, 2048, without quotes; a name in quotes. The comment next to each blank says where the value comes from.
- TODO 3 is in AniraProcessor.cpp: four blanks, marked fill out.
- Then build step09_exercise_check and run it. It names what is still missing, with the line.
- You are done when it matches the reference. Then the same processor runs as a plugin: step09_exercise_Standalone.

---

<div class="tag">Solution</div>

## TODO 1 · Declare the tensors

```json
"inputs": [
  { "name": "audio", "role": "streamed",
    "axes": [["batch",1],["channel",1],["time",2048]] },
  …
],
"outputs": [
  { "name": "audio_out", "role": "streamed",
    "axes": [["batch",1],["channel",1],["time",2048]], "latency": 2224 },
  …
]
```

Note:
- TODO 1 is the model's audio.
- Audio in and out have a time axis of 2048: one model block, as in step 1.
- On audio_out the model's own latency, 2224, as in step 7.
- If a size is wrong, prepare refuses the file and names the tensor.
---

<div class="tag">Solution</div>

## TODO 2 · Declare the state

```json
{ "name": "state_in",  "role": "state", "axes": [["batch", 1], ["any", 63488]],
  "state_source": "state_out" },
…
{ "name": "state_out", "role": "state", "axes": [["batch", 1], ["any", 63488]] }
```

Note:
- The state: 63,488 floats in and out, the size from step 2, and the pairing is one field, state_source: C, state_out.
- audio does not exist as an output, and audio_out is streamed audio: create refuses both, and says why.
- Try leaving it out once: create refuses the file, because nothing feeds that state.
- With the pair in place, anira knows each call depends on the one before.
- So it never runs two blocks of this model at the same time, and it keeps them in order.

---

<div class="tag">Solution</div>

## TODO 3 · Prepare and process

```cpp
// prepare()
m_contract.hard_host_blocks(1, max_block_size, sample_rate);   // 1 up to the largest
m_handler->prepare(m_contract);                                 // warm-up runs here
m_latency = m_handler->get_latency(0);                          // output slot 0

// process(): offline waits for the result; live, one call, in place
if (offline) { status = m_handler->process_wait(block, 0, block, 0, ANIRA_WAIT_FOREVER); }
else         { status = m_handler->process(block, 0, block, 0); }
```

Note:
- Given: the constructor reads the model file, drops its comments, and builds the pipeline and the handler. The prepare call is given too; the warm-up and the measurement run inside it.
- Blank 1: give the contract the host's blocks, any size from 1 up to its largest, and its sample rate.
- Blank 2: ask for the latency of output 0. On my machine it is 5295: the budget prepare measured, about 12.7 ms, for any host block up to 512. Yours depends on your machine.
- Blank 3, offline: when the host bounces, process_wait with ANIRA_WAIT_FOREVER waits for every result, so nothing is missed.
- Blank 4, live: in the audio callback, process over the block, in place. It never waits.
- The same four values in both calls. The check runs offline first, so its comparison is exact, and then once live.

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
- That is Steps 3, 6 and 7 in two files.
- There is no ring buffer, no worker, no queue, no state copy and no latency arithmetic in AniraProcessor.cpp, and the check from Step 7 passes.
- Next we take the model apart.
