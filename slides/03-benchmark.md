<h1>Step 3<br>Benchmarking and<br>the real-time budget</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
    - How long does it take, and how long in the worst case?
    - Ask the room: who measures worst case rather than average? What do you measure with?

**Script:**

*Ask the room: "Who measures worst case rather than average? What do you measure with?"*
"To know whether we can run the model in time, we need to measure how long a forward pass takes. We will do that with Google Benchmark, which is a library for measuring code performance. It will give us the average time, but also the worst case, which is what we care about."

---

<div class="tag">Motivation</div>

## The problem<br><span class="then">Every callback has a deadline.</span>

<div class="budget-calcs">
  <div class="budget-calc">
    <div><small>host block</small>64 samples</div>
    <b>÷</b>
    <div><small>sample rate</small>44.1 kHz</div>
    <b>=</b>
    <div class="result"><small>deadline per callback</small>1.45 ms</div>
  </div>
  <div class="budget-calc">
    <div><small>model block</small>2048 samples</div>
    <b>÷</b>
    <div><small>sample rate</small>44.1 kHz</div>
    <b>=</b>
    <div class="result"><small>deadline per block</small>46.4 ms</div>
  </div>
  <div class="budget-calc load">
    <div><small>forward pass</small>7.4 ms</div>
    <b>÷</b>
    <div><small>32 callbacks, one model block</small>46.4 ms</div>
    <b>=</b>
    <div class="result"><small>average load · what a CPU meter shows</small>16 %</div>
  </div>
  <div class="budget-calc load over">
    <div><small>forward pass</small>7.4 ms</div>
    <b>÷</b>
    <div><small>the callback that runs it</small>1.45 ms</div>
    <b>=</b>
    <div class="result"><small>load in callback 32 · what you hear</small>510 %</div>
  </div>
</div>

<div class="deadline">
  <div class="deadline-budget"><span>deadline</span></div>
  <div class="deadline-bars"><i style="--i:0"></i><i style="--i:1"></i><i style="--i:2"></i><i style="--i:3"></i><i style="--i:4"></i><i style="--i:5"></i><i style="--i:6"></i><i style="--i:7"></i><i style="--i:8"></i><i style="--i:9"></i><i style="--i:10"></i><i style="--i:11"></i><i style="--i:12"></i><i style="--i:13"></i><i style="--i:14"></i><i style="--i:15"></i><i style="--i:16"></i><i style="--i:17"></i><i style="--i:18"></i><i style="--i:19"></i><i style="--i:20"></i><i style="--i:21"></i><i style="--i:22"></i><i style="--i:23"></i><i style="--i:24"></i><i style="--i:25"></i><i style="--i:26"></i><i style="--i:27"></i><i style="--i:28"></i><i style="--i:29"></i><i style="--i:30"></i><i class="runs" style="--i:31"><span>forward pass <br> 7.4 ms</span></i></div>
  <div class="deadline-axis"><span>callbacks 1–31 · only copies samples, ≈ 0 ms</span><span>callback 32 · 2048 collected, run the model</span></div>
</div>

Note:
    - budget = host block size / sample rate: 64 samples at 44.1 kHz are 1.45 ms; 512 are 11.6 ms; 2048 are 46.4 ms.
    - 31 callbacks only fill the ring buffer from step 2. The 32nd has a full block of 2048 samples and runs the whole forward pass: about 7.4 ms on this laptop, in a 1.45 ms budget.
    - Spread over all 32 callbacks, 46.4 ms of audio, that is 16 %: what a CPU meter shows. In the callback that runs it, 510 %: a click every 46.4 ms.
    - This is the slide to linger on: the audio duration of the model block and the deadline of a callback are unrelated numbers.

**Script:**

"Every callback has a deadline, that we should not exceed. In the case of a host block size of 64 samples at 44.1 kHz gives you a budget of 1.45 milliseconds per callback. The model takes 2048 samples, at 44.1kHz that is 46 milliseconds of audio. So the budget of the model is 46 ms, but the deadline for each callback is only 1.45 ms.
The actual forward pass on my mac takes 7.4 ms, which is only about one sixth of the budget of the model, but it is 5 times the budget of a callback on the host's side. The first 31 callbacks of the host just copy samples, which costs virtually no time. The 32nd has to run the whole network, and it still only has 1.45 ms. Spread over all 32 callbacks that's 16 percent, which is what a CPU meter shows. But in the callback that runs it, it's 510 percent, and that's what you hear: a click every 46 milliseconds. So as we can see, we need to measure the forward pass and see how it compares to the budget of the host."

*Linger here. This is the key insight of the first half.*

---

<div class="tag">Goal</div>

## The goal<br><span class="then">Find out how long one call takes.</span>

<div class="statement-points">
  <div><small>1</small>Run the model inside the timed loop</div>
  <div><small>2</small>Time one call per repetition</div>
  <div><small>3</small>Read the worst case</div>
</div>

Note:
**Script:**

"So let's measure it properly. To do that we will have to run the model inside a loop that we time, one call at a time, and then read the slowest time the model takes for a block."

---

<div class="tag">Given</div>

## What we get <span class="then">1 new library, 1 file to edit.</span>

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">models/ & common/assets/</div>
    {{FILE:model.pt}}
    {{FILE:test_signal.h}}
  </div>
  <div class="given-column">
    <div class="given-label">Helpers</div>
    <div class="given-folder">common/helpers/ & CMake</div>
    {{FILE:libtorch_engine.h}}
    {{FILE:Google Benchmark|new}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">03_benchmark/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
    - Google Benchmark is fetched and built by CMake, the same library anira benchmarks with.
    - The input is the first 2048 samples of test_signal.h. The model is loaded once, before anything is timed: loading is not what we measure.
    - main.cpp: two TODOs, k_repetitions = 200.

**Script:**

"For this we will use Google Benchmark, fetched by CMake. It's what anira benchmarks with too. The model is loaded before timing, because loading isn't what we want to measure."

---

<div class="tag">Helpers: Google Benchmark</div>

## Google Benchmark <span class="then">Only the loop is timed.</span>

```cpp
void forward_pass(benchmark::State& state) {
    for (auto _ : state) { /* only this is timed */ }
}

BENCHMARK(forward_pass)
    ->Iterations(n)      // calls per repetition — otherwise it picks, and averages
    ->Repetitions(n)     // how often the whole measurement runs
    ->ComputeStatistics("p95", fn);   // our own statistic over the repetitions
```

Note:
    - Mean, median and standard deviation come for free. p95, p99 and max we add.

**Script:**

"Only the body of the loop is timed. By default it runs that body as often as it
likes and gives you the average. We want the opposite: one call per
repetition, many repetitions, and our own maximum time."

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="3"></div>

Fill in the TODOs in `steps/03_benchmark/exercise/main.cpp`

```bash
cmake --build --preset release --target step03_exercise
./build/bin/step03_exercise
```

<div class="run" style="grid-template-columns: 1fr 1fr">
  <div class="run-step" data-seconds="2.6">
    <div class="run-title"><span>1</span>The timed loop</div>
    <div class="run-visual run-timed">
      <div class="stopwatch"><i></i></div>
      <div class="mini-model">process()</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>2</span>One call per repetition</div>
    <div class="run-visual run-hist">
      <div class="hist-bars"><i style="--h:12%;--i:0"></i><i style="--h:38%;--i:1"></i><i style="--h:82%;--i:2"></i><i style="--h:100%;--i:3"></i><i style="--h:58%;--i:4"></i><i style="--h:26%;--i:5"></i><i style="--h:10%;--i:6"></i><i style="--h:0%;--i:7"></i><i style="--h:0%;--i:8"></i><i style="--h:9%;--i:9"></i></div>
      <div class="hist-max">max</div>
      <div class="run-ok">✓ the worst case, measured</div>
    </div>
  </div>
</div>

Note:
**Script:**

*Timebox.*
"So to summarize, there are two TODOs: we time a loop which processes one block, and we want this one call per repetition. Then we can read the slowest call in the results. You have 3 minutes to try and solve the task."

---

<div class="tag">Solution</div>

## TODO 1: The timed loop

```cpp
for (auto _ : state) {
    engine().process(block.data(), block_size); // <- fill out
    benchmark::DoNotOptimize(block.data());
}
```

Note:
    - Only the call itself: copying, allocating or printing in here is measured too.

**Script:**

"It's important to only time the call. Anything else in there gets measured too. We need `DoNotOptimize` to keep the compiler from deleting our work."

---

<div class="tag">Solution</div>

## TODO 2: One call per repetition

```cpp
BENCHMARK(forward_pass)
    ->Iterations(1) // <- fill out
    ->Repetitions(k_repetitions) // <- fill out
    // ...
```

Note:
    - slowest() and percentile() are given: the largest entry, and the entry a fraction of the way through a sorted copy.
    - Left alone, Google Benchmark runs the body as often as it likes and reports the average: the one number that cannot show a worst case.

**Script:**

"We need `Iterations(1)`. Otherwise we'll get an average, and an average can't show you
a worst case. Finding the slowest call and the percentiles is given."

---

<div class="tag">Done</div>

## You know your worst case<br><span class="then">Now it has to fit the budget.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
    - Run it three times. The max is far above the mean, and it is not random: it is the very first call, about 60 ms on this laptop. The second is about 40 ms; after that about 7.5 ms, with a worst case of about 10-12 ms.
    - mean / median: what a CPU meter shows. p95 / p99: what most users hit sometimes. max: what decides whether you shipped a broken plugin.
    - The first calls are warm-up: TorchScript optimises the model while it runs it (step 4 counts the allocations). They do not fit the 46.4 ms of a model block, so make them before the audio starts.
    - After that: page faults, the allocator, a thermal step, the scheduler. The worst case has to fit the budget, with room to spare: everything else in the host is fighting for that deadline too.
    - Measured on an ordinary thread, with nothing competing. In a callback, allocations, locks and page faults are not slow but fatal: step 4.

**Script:**

*Have people run it three times.*
"As we can see in the results, the max is far above the mean: around 60 milliseconds against 8. And it isn't random. It's the very first call, every time. TorchScript optimises the model while it runs it, so the first call is about 60 ms, the second about 40, and after that we're at about 7.5, with a worst case of 10 to 12. Those first calls don't even fit the 46 ms of a whole model block, so a plugin has to make them before the audio starts. The max is what
decides if your plugin is going to work under the worst-case scenario, so the worst case has to fit the budget, with room to spare. Mean is your CPU meter. p99 means one call in a hundred is slower than this, and at this block size that is about every 4.6 seconds. Also, this was measured on a quiet thread with nothing competing. On the audio thread, allocations and locks aren't just slow. They're fatal. We will talk more about this in the next step."
