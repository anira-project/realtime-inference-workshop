<h1>Step 4<br>Benchmarking and<br>the real-time budget</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

Note:
- In this step we'll get into benchmarking and talk about if we're real time capable at all.
- Measure how long a forward pass takes
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
- As you all know in real-time programming it's important to be in time. This means we have a deadline for how long our processing is allowed to take.
- the real-time threshold, how much time we have, so our budget
- Model: 2048 samples = threshold of 46 ms
- Model block’s threshold says whether real-time is possible at all
- 512 samples at 44.1 kHz: threshold of 11.6 ms per callback
- The host callback’s threshold says whether this synchronous implementation actually makes it. So if we can manage one forward pass in one callback from the host.
- Click: real-time factor, how much of that time (our budget) we use; has to stay below 1
- Forward pass on ONNX Runtime on my Mac: the average is about 13 ms
- Against the model's threshold: 0.28, what a CPU meter shows. 
- It looks fine, but it's only an average over the callbacks.
- Compared to the callback's threshold: 1.12. 
- Too long, even on average (13 ms is not even the worst case). For a slow call of 18 ms: 1.55. what you hear: a dropout
- Click: 3 callbacks just copy samples, the 4th runs the whole network in 13ms
- We are realtime capable, but not fast enough for one callback of the host. 
- And this is only looking at the average, not the worst case.
- The average doesn't decide, the worst case does: measure it

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
- Run it again: what stays the same?

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
- Let's take a look

---

<div class="tag">Timing: <code>std::chrono</code></div>

## Timing <span class="then">Only the call is timed.</span>

```cpp
constexpr size_t k_iterations = 10;  // 10 forward passes each, every one timed
constexpr size_t k_repetitions = 2;  // 2 fresh engines, as in anira's benchmarks

for (size_t repetition = 0; repetition < k_repetitions; ++repetition) {
    OnnxEngine engine(k_model.m_path);  // not timed

    for (size_t iteration = 0; iteration < k_iterations; ++iteration) {
        const auto begin = std::chrono::steady_clock::now();
        engine.process(model_block.data(), model_block.size(), state);  // only this is timed
        const auto end = std::chrono::steady_clock::now();
        times_ms[repetition][iteration] = std::chrono::duration<double, std::milli>(end - begin).count();
    }
}
```

Note:
- Only the forward pass between the two clock reads: that's what the worker in step 6 runs
- `steady_clock`: a monotonic clock, made for measuring intervals
- Every forward pass timed on its own: an average can't show a worst case

- Iterations: how many forward passes will be measured in 
- Repetitions: creates a new onnx runtime environment and loads the model into a new session -> fresh engine each time

---

<div class="tag">Run it</div>

## Run it <span class="then">Are we real-time capable?</span>

<div class="task-timer" data-minutes="2"></div>

Nothing to edit, `steps/04_benchmark/solution/main.cpp` is ready to run

```bash
cmake --build --preset release --target step04_solution
./build/bin/step04_solution
```

Note:
- Nothing to edit: build it and run it
- You can change the amount of repetitions and iterations to see what happens
- 2 minutes

---

<div class="tag">Result</div>

## What it prints

<div class="output-full">

```text
Real-time threshold at 44100 Hz:
  model block   2048 samples    46.44 ms
  callback       512 samples    11.61 ms

The first 5 forward passes of 2 repetitions, in ms:
               rep 1     rep 2
  pass  1     18.00     13.59
  pass  2     14.14     13.05
  pass  3     13.25     13.07
  pass  4     13.40     12.97
  pass  5     12.92     12.97

All 20 forward passes:
  mean      13.39 ms
  median    13.07 ms
  min       12.89 ms
  p99       18.00 ms
  max       18.00 ms

The 5 slowest of all 20 forward passes:
  repetition 1, pass   1   18.00 ms
  repetition 1, pass   2   14.14 ms
  repetition 2, pass   1   13.59 ms
  repetition 1, pass   7   13.46 ms
  repetition 1, pass   4   13.40 ms

Your worst case:
  max inference time            18.00 ms = 794 samples
  rounded_max_inference_time   1024 samples (whole 512-sample callbacks)
```

</div>

Note:
- Screen share on (C): run it live in the IDE's terminal
- Without a share: this slide shows the output of one earlier run, on ONNX Runtime
- Look at the passes of both repetitions, 
- We also see some general statistics of our runs, mean, p99, max
- Mean: is what your CPU meter shows
- p99: one call in a hundred is slower. With only 20 passes that is simply the slowest one, so here p99 = max = 18 ms
- Then at bottom a list of the 5 slowest
- The last lines: your worst case, over all the forward passes. 
- Write it down, steps 6 and 7 need it

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
- The run from the previous slide: repetition 1, the one with the worst pass
- First pass 18 ms, second 14.1 ms, then about 13 ms
- Click, the two thresholds together: pass 1, 18.00 ms, factor 0.39 against a model block, 1.55 against a callback
- It fits the 46 ms of a model block easily
- It doesn't fit the 11.6 ms of a callback, not even a typical call

---

<div class="tag">Result</div>

## LibTorch<br><span class="then">A long warm-up, then faster.</span>

<div class="warmup">
  <div class="warmup-line model fragment" data-fragment-index="1" style="--at:0.516"><span>model block<br>46.4 ms</span></div>
  <div class="warmup-line callback fragment" data-fragment-index="1" style="--at:0.129"><span>512-sample callback<br>11.6 ms</span></div>
  <div class="warmup-bars"><i class="warm" style="--h:88.5%;--i:0"><span>79.6 ms</span><em>1</em></i><i class="warm" style="--h:42.7%;--i:1"><span>38.4 ms</span><em>2</em></i><i class="" style="--h:8.7%;--i:2"><span>7.8 ms</span><em>3</em></i><i class="" style="--h:8.7%;--i:3"><span>7.9 ms</span><em>4</em></i><i class="" style="--h:9.2%;--i:4"><span>8.3 ms</span><em>5</em></i><i class="" style="--h:9.1%;--i:5"><span>8.2 ms</span><em>6</em></i><i class="" style="--h:8.4%;--i:6"><span>7.6 ms</span><em>7</em></i><i class="" style="--h:8.5%;--i:7"><span>7.6 ms</span><em>8</em></i><i class="" style="--h:8.5%;--i:8"><span>7.6 ms</span><em>9</em></i><i class="" style="--h:8.3%;--i:9"><span>7.5 ms</span><em>10</em></i></div>
  <div class="warmup-axis">call</div>
</div>

Note:
- We don't use Libtorch in this workshop, but just for comparison
- The same model, different engine on LibTorch
- First call 80 ms, second 38 ms: TorchScript optimises the model while it runs it
- Click: from the third call on about 7.7 ms, faster than ONNX Runtime
- The two thresholds: after warm up a typical call fits the callback, with little room
- It's the first calls every time that are slower, not random
- The slower first passes are warm-up: the first calls after loading do one-time setup
- This also happens with onnx runtime, but the warm up time is much faster

---

<div class="tag">Result</div>

## The two engines<br><span class="then">One run each.</span>

| | first call | second call | typical | slowest |
|---|---|---|---|---|
| ONNX Runtime | 18.0 ms | 14.1 ms | 13 ms | 18.0 ms |
| LibTorch | 79.6 ms | 38.4 ms | 7.7 ms | 79.6 ms |


Note:
- Same model, same machine, very different shape
- ONNX Runtime: short warm-up, slower typical call
- LibTorch: long warm-up, faster typical call
- The slowest call changes from run to run on both, and gets much worse when the machine is busy: run it again and compare
- We continue with ONNX Runtime, so that's the number you write down
- We leave the warm up out in part 1: even the warm-up stays far below 46.4 ms, most likely on your machine too, so we don't consider it relevant.

---

<div class="tag">Question</div>

## Anyone above 46.4 ms?<br><span class="then">Then the model cannot keep up.</span>

<div class="big-question">?</div>

Note:
- Ask the room: did anyone measure a worst case above 46.4 ms?
- That means one forward pass would take longer than the audio it produces: no latency fixes that
- Run it again with other apps closed and the laptop on power; a busy machine makes the worst case worse
- Still above: tell us, we look at it together

---

<div class="tag">Budget</div>

## Your number goes into one header<br><span class="then">Steps 6, 7 and the plugin read it.</span>

<div class="task-timer" data-minutes="1"></div>

```cpp
// steps/common/inference_time.h

// One model block of time: 2048 samples at 44.1 kHz
constexpr double k_worst_case_inference_ms = 46.4;

// Your worst case, from step 4
constexpr double k_max_inference_ms = 18.0;  // <- fill out after step 4
```

Until you fill it in, it is 46.4 ms: everything runs, with more latency.<br>
Above 46.4 ms, the build stops.

Note:
- One place for your number: `steps/common/inference_time.h`
- The last line of the output says which number to put there
- Steps 6, 7 and the plugin read it from there
- Above it: 46.4 ms, one model block of time, the largest worst case that can work at all
- Until you put yours in, it is 46.4: everything runs, with more latency
- Yours makes the latency smaller
- Larger than 46.4: the build stops with a message

---

<div class="tag">Budget</div>

## Your worst case<br><span class="then">In samples, in whole callbacks.</span>

<div class="lat-add">
  <div class="lat-term"><span>max inference time</span><small>over all forward passes</small><strong>18.0 ms</strong></div>
  <div class="lat-op">=</div>
  <div class="lat-term fragment" data-fragment-index="1"><span>in samples</span><small>× 44.1 kHz</small><strong>794</strong></div>
  <div class="lat-op fragment" data-fragment-index="2">→</div>
  <div class="lat-term total fragment" data-fragment-index="2"><span>rounded_max_inference_time</span><small>whole 512-sample callbacks</small><strong>1024</strong></div>
</div>

Note:
- `inference_time.h` (next slide) works this out from your number: in samples, and rounded to whole callbacks
- Your worst call, from the last lines of the output
- Click: in samples, at 44.1 kHz: 18 ms are 794 samples
- Click: rounded up to whole 512-sample callbacks: 1024, two callbacks
- Whole callbacks, because the audio thread only picks results up once per callback
- You don't need to do this, but you will need this variable later in step 6 and 7

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
- Typically about 13 ms: it fits a 46 ms block easily, but not a 11.6 ms callback
- Measured on a quiet thread, nothing competing
- So we are real-time capable, but we currently don't fit our hosts callback.
- The next question is, are we even real-time safe?
