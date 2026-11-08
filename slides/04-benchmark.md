<h1>Step 4<br>Benchmarking and<br>the real-time budget</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

Note:
- Let's measure this in our next step
- In this step we're going to define our realtime budget and do some benchmarking 
- No code to write in this step: run on your own machine

---

<div class="tag">Motivation</div>

## The problem<br><span class="then">Every callback has a deadline.</span>

<div class="budget-tables">
  <div class="budget-group">
    <div class="budget-question"><strong>Real-time threshold</strong>How much time do we have?</div>
    <table class="budget-table">
      <thead><tr><th>Block</th><th>÷ Sample rate</th><th>= Threshold</th></tr></thead>
      <tbody>
        <tr><td>Model: 2048 samples</td><td>44.1 kHz</td><td class="result">46.4 ms</td></tr>
        <tr><td>Host: 512 samples</td><td>44.1 kHz</td><td class="result">11.6 ms</td></tr>
      </tbody>
    </table>
  </div>
  <div class="budget-group fragment" data-fragment-index="1">
    <div class="budget-question"><strong>Real-time factor</strong>How much of that time do we use? Must stay below 1.</div>
    <table class="budget-table factor">
      <thead><tr><th>Forward pass</th><th>÷ Threshold</th><th>= Factor</th><th></th></tr></thead>
      <tbody>
        <tr><td>13 ms</td><td>46.4 ms <small>model block</small></td><td class="result">0.28</td><td class="meaning">What a CPU meter shows</td></tr>
        <tr><td>13 ms</td><td>11.6 ms <small>host callback</small></td><td class="result over">1.12</td><td class="meaning">Too long, on average</td></tr>
      </tbody>
    </table>
  </div>
</div>

<div class="deadline fragment" data-fragment-index="2">
  <div class="deadline-budget"><span>real-time threshold · 11.6 ms</span></div>
  <div class="deadline-bars"><i style="--i:0"></i><i style="--i:1"></i><i style="--i:2"></i><i class="runs" style="--i:3;height:33.5%"><span>forward pass<br>13 ms</span></i></div>
  <div class="deadline-axis"><span>callbacks 1–3 · only copy samples, ≈ 0 ms</span><span>callback 4 · 2048 collected, run the model</span></div>
</div>

Note:
- Go through the slide fast
- Every callback has a deadline: the real-time threshold, our time budget
- Model block: 2048 samples = 46.4 ms. Says whether real-time is possible at all
- Host callback: 512 samples = 11.6 ms. Tells us the time we have to fit one forward pass into one callback
- Click: real-time factor = forward pass ÷ threshold, must stay below 1
- ONNX Runtime on my Mac, about 13 ms on average: 0.28 against the model block (what a CPU meter shows), 1.12 against the callback: too long
- Click: 3 callbacks only copy samples, the 4th runs the whole model
- And that's only the average.

---

<div class="tag">Goal</div>

## The goal<br><span class="then">Find out how long one call takes on your machine.</span>

<div class="statement-points">
  <div><small>1</small>Run the benchmark</div>
  <div><small>2</small>Read your numbers</div>
  <div><small>3</small>Check the worst case</div>
</div>

Note:
- Everything is written: you run it and read your own numbers
- One call at a time, then the slowest time for a block

---

<div class="tag">Given</div>

## What we get <span class="then">Nothing new, nothing to edit.</span>

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">common/assets/</div>
    {{FILE:model_forward.onnx}}
    {{FILE:test_signal.h}}
  </div>
  <div class="given-column">
    <div class="given-label">Helpers</div>
    <div class="given-folder">common/helpers/</div>
    {{FILE:onnx_engine.h}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Ready to run</div>
    <div class="given-folder">04_benchmark/solution/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
- The ONNX Runtime engine and its state from step 2, timed with `std::chrono`
- Model is loaded before timing: loading isn't what we measure
- We only measure our forward pass
- Let's take a look how we measure this

---

<div class="tag">Timing: <code>std::chrono</code></div>

## Timing <span class="then">Only the call is timed.</span>

```cpp
constexpr size_t k_iterations = 10;  // 10 forward passes, every one timed
constexpr size_t k_repetitions = 2;  // 2 fresh engines
for (size_t repetition = 0; repetition < k_repetitions; ++repetition) {
    OnnxEngine engine;
    engine.load(k_model.m_path);  // not timed

    for (size_t iteration = 0; iteration < k_iterations; ++iteration) {
        const auto begin = std::chrono::steady_clock::now();
        engine.process(model_block.data(), k_model.m_input_size,
                       state.data(), k_model.m_state_size);  // only this is timed
        const auto end = std::chrono::steady_clock::now();
        times_ms[repetition][iteration] =
            std::chrono::duration<double, std::milli>(end - begin).count();
    }
}
```

Note:
- Only the forward pass between the two clock reads: that's what the worker in step 6 runs
- `steady_clock`: a monotonic clock, made for measuring intervals
- Every forward pass timed on its own: an average can't show a worst case
- Iterations: how many forward passes are timed within one session.
- Repetitions: how many times the whole benchmark runs from scratch. Each repetition creates a new ONNX Runtime environment and a new session, so nothing is carried over between runs.

---

<div class="tag">Result</div>

## ONNX Runtime<br><span class="then">One slower call, then flat, above the callback.</span>

<div class="warmup">
  <div class="warmup-line model fragment" data-fragment-index="1" style="--at:0.516"><span>model block<br>46.4 ms</span></div>
  <div class="warmup-line callback fragment" data-fragment-index="1" style="--at:0.129"><span>512-sample callback<br>11.6 ms</span></div>
  <div class="warmup-bars"><i class="warm" style="--h:20.0%;--i:0"><span>18.00 ms</span><em>1</em></i><i class="" style="--h:15.7%;--i:1"><span>14.14 ms</span><em>2</em></i><i class="" style="--h:14.7%;--i:2"><span>13.25 ms</span><em>3</em></i><i class="" style="--h:14.9%;--i:3"><span>13.40 ms</span><em>4</em></i><i class="" style="--h:14.4%;--i:4"><span>12.92 ms</span><em>5</em></i></div>
  <div class="warmup-axis">call</div>
</div>

Note:
- One earlier run on my Mac, ONNX Runtime: first pass 18 ms, then about 13 ms
- First iteration is slower, this is typical for inference engines, because they have some warm up time
- Click, the two thresholds: every call fits the 46.4 ms model block easily
- None fits the 11.6 ms callback, not even a typical one
---

<div class="tag">Run it</div>

## Run it <span class="then">Are we real-time capable?</span>

<div class="task-timer" data-minutes="2"></div>

Nothing to edit, `steps/04_benchmark/solution/main.cpp` is ready to run

```bash
cmake --build --preset release --target step04_solution
./build/release/bin/step04_solution
```

Note:
- Nothing to edit: build it and run it
- You can change the amount of repetitions and iterations to see what happens
- Take a look at the results
- 2 minutes
- Remeber the output of your worst case, we need it in steps 6 and 7


---

<div class="tag">Question</div>

## Anyone above 46.4 ms?<br><span class="then">Then the model cannot keep up.</span>

<div class="big-question">?</div>

Note:
- Ask the room: did anyone measure a mean above 46.4 ms?
- Colleague will come to have a look and help
- That means one forward pass would take longer than the audio it produces: no latency fixes that
- Run it again with other apps closed and the laptop on power; a busy machine makes the worst case worse
- Still above: tell us, we look at it together

---

<div class="tag"><code>inference_time.h</code></div>

## Your max inference time goes into one header<br><span class="then">Steps 6, 7 and the plugin read it.</span>

<div class="task-timer" data-minutes="1"></div>

```cpp
// steps/common/inference_time.h
constexpr double k_max_inference_ms = «18.0»;  // <- fill out after step 4
```

Until you fill it in, it is 46.4 ms, one model block of time: everything runs, with more latency.<br>
Above 46.4 ms, the build stops.

Note:
- One place for your number: `steps/common/inference_time.h`
- The last line of the output says which number to put there
- Steps 6, 7 and the plugin read it from there
- Until you put yours in, it is 46.4: everything runs, with more latency
- Yours makes the latency smaller
- Larger than 46.4: the build stops with a message

---


<div class="tag">Done</div>

## You know your worst case

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
- So what did we find out?
- Our worst case. In my case 18 ms: it fits a 46 ms block easily, but not a 11.6 ms callback
- Measured on a quiet thread, nothing competing
- So we are real-time capable, but we currently don't fit our hosts callback.

