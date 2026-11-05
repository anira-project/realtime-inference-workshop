<h1>Step 4<br>Benchmarking and<br>the real-time budget</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

Note:
- Ask: who measures worst case rather than average? What do you measure with?
- No code to write this step: a demo, run on your own machine
- Measure how long a forward pass takes
- One forward pass at a time, timed with `std::chrono`: the average, but also the worst case
- The worst case is what we care about

**Script:**

*Ask the room: "Who measures worst case rather than average? What do you measure with?"*
"To know whether we can run the model in time, we need to measure how long a forward pass takes. We time one forward pass at a time with `std::chrono`. That gives us the average time, but also the worst case, which is what we care about. There is no code to write in this step: you just run it and read the numbers from your own machine."

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
        <tr><td>15.3 ms <small>a slow call</small></td><td>11.6 ms <small>host callback</small></td><td class="result over">1.32</td><td class="meaning">What you hear</td></tr>
      </tbody>
    </table>
  </div>
</div>

<div class="deadline fragment" data-fragment-index="2">
  <div class="deadline-budget"><span>real-time threshold · 11.6 ms</span></div>
  <div class="deadline-bars"><i style="--i:0"></i><i style="--i:1"></i><i style="--i:2"></i><i class="runs" style="--i:3;height:33.5%"><span>a slow call<br>15.3 ms</span></i></div>
  <div class="deadline-axis"><span>callbacks 1–3 · only copy samples, ≈ 0 ms</span><span>callback 4 · 2048 collected, run the model</span></div>
</div>

Note:
- Every callback has a deadline: the real-time threshold, how much time we have
- 512 samples at 44.1 kHz: threshold of 11.6 ms per callback
- Model: 2048 samples = threshold of 46 ms
- Click: real-time factor, how much of that time we use; has to stay below 1
- Forward pass on ONNX Runtime on my Mac: about 13 ms
- Against the model's threshold: 0.28, what a CPU meter shows
- Against the callback's threshold: 1.12. Too long, even on average
- A slow call, 15.3 ms: 1.32, what you hear: a dropout
- Click: 3 callbacks just copy samples, the 4th runs the whole network in 11.6 ms
- The average doesn't decide, the worst case does: measure it
- Linger here: key insight of the first half

**Script:**

"Every callback has a deadline that we should not exceed. We call that the real-time threshold: how much time we have. It's the block size divided by the sample rate. Our host block of 512 samples at 44.1 kHz gives us 11.6 milliseconds per callback. The model takes 2048 samples, which is 46 milliseconds of audio.

*Click: the real-time factor.*

The real-time factor is how much of that time we use: the processing time divided by the threshold. It has to stay below 1, because above 1 we're too late. The forward pass on ONNX Runtime on my Mac takes about 13 milliseconds. Against the model's threshold that's a factor of 0.28, which is what a CPU meter shows. Against the callback's threshold it's 1.12: that's too long, even on average. And not every call takes 13 milliseconds: a slow one of 15.3 milliseconds is a factor of 1.32, and that's what you hear, a dropout.

*Click: the bars.*

Why does the callback count? The first three callbacks of the host just copy samples, which costs virtually no time. The fourth has to run the whole network, and it still only has 11.6 milliseconds. So the average doesn't decide whether we make it, the worst case does. That's why we need to measure the forward pass, every single call, and compare it to the threshold of the host."

*Linger here. This is the key insight of the first half.*

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

**Script:**

"So let's measure it properly. The program is ready: it runs the model one call at a time and shows you how long each call took on your machine. Then we run it again and see what stays the same."

---

<div class="tag">Given</div>

## What we get <span class="then">Nothing new, nothing to edit.</span>

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">models/ & common/assets/</div>
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

**Script:**

"We take the engine from step 2, with its state, and time the forward pass with `std::chrono` from the standard library. The model is loaded before timing, because loading isn't what we want to measure."

---

<div class="tag">Timing: <code>std::chrono</code></div>

## Timing <span class="then">Only the call is timed.</span>

```cpp
constexpr size_t k_repetitions = 5;   // 5 fresh engines, as in anira's benchmarks
constexpr size_t k_iterations = 200;  // 200 forward passes each, every one timed

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
- Repetitions: a fresh engine each time, so we see whether the slow passes come back with every engine
- Same structure as anira's benchmarks: a fresh `InferenceHandler` per repetition, every `process()` timed

**Script:**

"Only the forward pass stands between the two clock reads, because that's what the worker in step 6 will run. We use the steady clock, because it's monotonic and made for measuring intervals. Every forward pass is timed on its own: an average can't show a worst case. And we do it five times, with a fresh engine each time, 200 forward passes each. That way we see which slow passes come back with every engine. That's the same structure anira's benchmarks use."

---

<div class="tag">Run it</div>

## Run it <span class="then">Are we real-time capable?</span>

<div class="task-timer" data-minutes="3"></div>

Nothing to edit, `steps/04_benchmark/solution/main.cpp` is ready to run

```bash
cmake --build --preset release --target step04_solution
./build/bin/step04_solution
```

Note:
- Nothing to edit: build it and run it
- Look at the first forward passes of every engine, then at the list of the 5 slowest
- The last lines: your worst case after the warm-up. Write it down, steps 6 and 7 need it
- Run it three times
- 2 minutes

**Script:**

*Timebox.*
"So, there's nothing to edit here. Please build it and run it. Have a look at which forward passes are slow in every engine, and at the list of the 5 slowest. The last lines print your worst forward pass after the warm-up: please write that number down, you'll need it in steps 6 and 7. Then run it three times and see what changes. You have 2 minutes."

---

<div class="tag">Solution</div>

<!-- TODO: replace the output below with a run of the current step04_solution (ONNX Runtime only, 5 engines x 200 passes). LibTorch is no longer in the program. -->

## What it prints

```text
ONNX Runtime, the first 10 of 200 calls:
  call  1  ########################################  18.97 ms
  call  2  ############################              13.24 ms
  ...
LibTorch, the first 10 of 200 calls:
  call  1  ########################################  79.62 ms
  call  2  ###################                       38.42 ms
  call  3  ####                                       7.84 ms
  ...

The two engines, after the warm-up:
               fastest   slowest
  ONNX Runtime  12.77 ms  15.30 ms
  LibTorch       7.45 ms  22.65 ms
```

Note:
- Screen share on (C): run it live in the IDE's terminal
- Without a share: this slide shows the output of one earlier run
- Look at the first calls together, then at the list of the 5 slowest
- Run it again: what stays the same, what changes?

**Script:**

*Run it live.*
"Let's run it together. Here are the first forward passes of every engine. Look at the first calls: they're slower than the rest, and much slower on LibTorch. And in the list of the 5 slowest calls, you'll find those same calls. Let's run it again and see what stays the same."

---

<div class="tag">Result</div>

## ONNX Runtime<br><span class="then">One slower call, then flat, above the callback.</span>

<div class="warmup">
  <div class="warmup-line model fragment" data-fragment-index="1" style="--at:0.516"><span>model block<br>46.4 ms</span></div>
  <div class="warmup-line callback fragment" data-fragment-index="1" style="--at:0.129"><span>512-sample callback<br>11.6 ms</span></div>
  <div class="warmup-bars"><i class="warm" style="--h:21.1%;--i:0"><span>19.0 ms</span><em>1</em></i><i class="" style="--h:14.7%;--i:1"><span>13.2 ms</span><em>2</em></i><i class="" style="--h:14.3%;--i:2"><span>12.9 ms</span><em>3</em></i><i class="" style="--h:14.5%;--i:3"><span>13.0 ms</span><em>4</em></i><i class="" style="--h:14.3%;--i:4"><span>12.8 ms</span><em>5</em></i><i class="" style="--h:14.5%;--i:5"><span>13.1 ms</span><em>6</em></i><i class="" style="--h:16.1%;--i:6"><span>14.5 ms</span><em>7</em></i><i class="" style="--h:17.0%;--i:7"><span>15.3 ms</span><em>8</em></i><i class="" style="--h:15.4%;--i:8"><span>13.9 ms</span><em>9</em></i><i class="" style="--h:15.4%;--i:9"><span>13.9 ms</span><em>10</em></i></div>
  <div class="warmup-note fragment" data-fragment-index="0" style="top:40%;bottom:auto">worst after warm-up: 15.3 ms</div>
  <div class="warmup-axis">call</div>
</div>

Note:
- One run on my Mac: the first 10 of 200 calls, on ONNX Runtime
- First call 19 ms, then about 13 ms
- Click: worst after warm-up, over all the calls: 15.3 ms, factor 0.33 against a model block, 1.32 against a callback
- The last lines of your output: the worst call after the warm-up
- Click: the two thresholds
- It fits the 46 ms of a model block easily
- It doesn't fit the 11.6 ms of a callback, not even a typical call
- Linger here

**Script:**

"This is one run on my Mac, the first 10 calls on ONNX Runtime. The first call takes 19 milliseconds, and after that it's about 13.

*Click: warmed up.*

Now look at the last lines in your terminal: that's your worst case after the warm-up. In this run, over all the calls, mine was 15.3 milliseconds. That's a factor of 0.33 against a model block and 1.32 against a 512-sample callback.

*Click: the thresholds.*

Now let's add our two thresholds. It fits easily into the 46 milliseconds of a model block. But it doesn't fit into the 11.6 milliseconds of a callback, not even a typical call."

---

<div class="tag">Result</div>

## LibTorch<br><span class="then">A long warm-up, then faster.</span>

<div class="warmup">
  <div class="warmup-line model fragment" data-fragment-index="1" style="--at:0.516"><span>model block<br>46.4 ms</span></div>
  <div class="warmup-line callback fragment" data-fragment-index="1" style="--at:0.129"><span>512-sample callback<br>11.6 ms</span></div>
  <div class="warmup-bars"><i class="warm" style="--h:88.5%;--i:0"><span>79.6 ms</span><em>1</em></i><i class="warm" style="--h:42.7%;--i:1"><span>38.4 ms</span><em>2</em></i><i class="" style="--h:8.7%;--i:2"><span>7.8 ms</span><em>3</em></i><i class="" style="--h:8.7%;--i:3"><span>7.9 ms</span><em>4</em></i><i class="" style="--h:9.2%;--i:4"><span>8.3 ms</span><em>5</em></i><i class="" style="--h:9.1%;--i:5"><span>8.2 ms</span><em>6</em></i><i class="" style="--h:8.4%;--i:6"><span>7.6 ms</span><em>7</em></i><i class="" style="--h:8.5%;--i:7"><span>7.6 ms</span><em>8</em></i><i class="" style="--h:8.5%;--i:8"><span>7.6 ms</span><em>9</em></i><i class="" style="--h:8.3%;--i:9"><span>7.5 ms</span><em>10</em></i></div>
  <div class="warmup-note fragment" data-fragment-index="0">worst after warm-up: 22.7 ms</div>
  <div class="warmup-axis">call</div>
</div>

Note:
- The same run, the same scale, on LibTorch
- LibTorch is not in the program any more: this is one earlier run, for comparison
- First call 80 ms, second 38 ms: TorchScript optimises the model while it runs it
- It's the first calls every time, not random
- Click: from the third call on about 7.7 ms, faster than ONNX Runtime
- Worst after warm-up in this run: 22.7 ms. That number changes from run to run, on both engines
- Click: the two thresholds: a typical call fits the callback, with little room

**Script:**

"And here's the same run on LibTorch, on the same scale. The first call takes 80 milliseconds, the second 38. And that's not random: you should see the same on your machine, every time you run it. TorchScript optimises the model while it runs it, so the first calls are a warm-up.

*Click: warmed up.*

From the third call on it's about 7.7 milliseconds, so warmed up it's faster than ONNX Runtime here. But in this run its slowest call was 22.7 milliseconds, and on both engines that number changes from run to run.

*Click: the thresholds.*

The very first call doesn't even fit a model block. A typical call does fit the callback, but with not much room to spare."

---

<div class="tag">Result</div>

## The two engines<br><span class="then">One run, 200 calls each.</span>

| | first call | second call | typical | slowest after warm-up |
|---|---|---|---|---|
| ONNX Runtime | 19.0 ms | 13.2 ms | 13 ms | 15.3 ms |
| LibTorch | 79.6 ms | 38.4 ms | 7.7 ms | 22.7 ms |

Neither may run in a 512-sample callback. **From here on: your ONNX Runtime number.**

Note:
- Same model, same machine, very different shape
- ONNX Runtime: short warm-up, slower typical call
- LibTorch: long warm-up, faster typical call
- The slowest call changes from run to run on both, and gets much worse when the machine is busy: run it again and compare
- We continue with ONNX Runtime, so that's the number you write down

**Script:**

"Side by side: it's the same model on the same machine, but the shape is very different. ONNX Runtime has a short warm-up and a slower typical call. LibTorch has a long warm-up and a faster typical call. The slowest call changes from run to run on both engines, and it gets much worse when the machine is busy. These numbers are from a single run, so run it again and compare. We continue with ONNX Runtime, so that's the number you write down."

---

<div class="tag">Budget</div>

## Your worst case<br><span class="then">In samples, in whole callbacks.</span>

<div class="lat-sum" style="margin-bottom: 4rem">
  <div class="lat-part">max inference time<small>after the warm-up</small><strong>15.3 ms</strong></div>
  <div class="lat-plus">=</div>
  <div class="lat-part fragment" data-fragment-index="1">in samples<small>× 44.1 kHz</small><strong>675</strong></div>
  <div class="lat-plus fragment" data-fragment-index="2">→</div>
  <div class="lat-part total fragment" data-fragment-index="2">rounded_max_inference_time<small>whole 512-sample callbacks</small><strong>1024</strong></div>
</div>

Note:
- Your worst call after the warm-up, from the last lines of the output
- Click: in samples, at 44.1 kHz: 15.3 ms are 675 samples
- Click: rounded up to whole 512-sample callbacks: 1024, two callbacks
- Whole callbacks, because the audio thread only picks results up once per callback
- That's how much time the worker gets in step 6, and part of the latency in step 7
- Everyone has their own number: it goes into a header, next slide
- Measured warmed up, so the plugin has to warm up before the audio starts

**Script:**

"Let's turn your worst case into the number we'll need. Mine was 15.3 milliseconds after the warm-up.

*Click.*

In samples that's 15.3 milliseconds times 44.1 kilohertz: 675 samples.

*Click.*

And we round that up to whole callbacks of 512 samples, so 1024: two callbacks. Whole callbacks, because the audio thread only picks results up once per callback. We call that the rounded max inference time. In step 6 it's the time the worker gets, and in step 7 it becomes part of our latency. Everyone has their own number here, and on the next slide we put yours in. And because we measured after the warm-up, our plugin will have to warm up before the audio starts."

---

<div class="tag">Budget</div>

## Your number goes into one header<br><span class="then">Steps 6, 7 and the plugin read it.</span>

<div class="task-timer" data-minutes="1"></div>

```cpp
// steps/common/inference_time.h

// One model block of time: 2048 samples at 44.1 kHz
constexpr double k_worst_case_inference_ms = 46.4;

// Your worst case after the warm-up, from step 4
constexpr double k_max_inference_ms = 15.3;  // <- fill out after step 4
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

**Script:**

"Your number goes into one place: `steps/common/inference_time.h`. The last line of the program even tells you which number to put there. Steps 6, 7 and the plugin read it from there. Above it you see 46.4 milliseconds: that's one model block of time, 2048 samples at 44.1 kHz, the largest worst case that can work at all. Until you put your number in, it's 46.4, so everything runs, just with more latency. Your number makes the latency smaller. If you put in something larger than 46.4, the build stops and tells you. Please take a minute and put your number in now."

---

<div class="tag">Question</div>

## Anyone above 46.4 ms?<br><span class="then">Then the model cannot keep up.</span>

<div class="big-question">?</div>

Note:
- Ask the room: did anyone measure a worst case above 46.4 ms?
- Then one forward pass takes longer than the audio it produces: no latency fixes that
- Run it again with other apps closed and the laptop on power; a busy machine makes the worst case worse
- Still above: tell us, we look at it together

**Script:**

*Ask the room.*
"Did anyone measure a worst case above 46.4 milliseconds? Then one forward pass takes longer than the 2048 samples of audio it produces, and no latency in the world fixes that: the model can't keep up on this machine. Run it again with other apps closed and the laptop plugged in, because a busy machine makes the worst case worse. If it's still above, let us know and we'll look at it together."

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
- A plugin makes the first calls before the audio starts
- Warmed up: about 13 ms, it fits a 46 ms block easily, but not a 11.6 ms callback
- Run it again: sometimes a call after the warm-up is slow too, one LibTorch run of mine had 43 ms, factor 0.93 against a block, 3.7 against a callback
- That one isn't warm-up: the worst case has to fit with room to spare
- In the callback it doesn't: the next steps take the model out of it
- Mean: your CPU meter
- p99: one call in a hundred is slower, here about every 4.6 s
- Measured on a quiet thread, nothing competing
- On the audio thread, allocations and locks are fatal: next step

**Script:**

"So what does this tell us? A plugin has to make those first calls before the audio starts. Once it's warmed up, our computer is fast enough for a model block, but not for a 512-sample callback.
If you ran it a few times, you may have seen something else: sometimes a call after the warm-up is slow too. One of my LibTorch runs had 43 milliseconds, which is nearly the whole model block, and almost four callbacks. That isn't warm-up, it's the rest of the system getting in the way, so the worst case has to fit with room to spare. Inside the callback it doesn't, and that's what the next steps solve. The mean is your CPU meter. p99 means one call in a hundred is slower than this, and at this block size that is about every 4.6 seconds. Also, this was measured on a quiet thread with nothing competing. On the audio thread, allocations and locks aren't just slow. They're fatal. We will talk more about this in the next step."
