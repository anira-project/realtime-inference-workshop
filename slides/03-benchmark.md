<h1>Step 3<br>Benchmarking and<br>the real-time budget</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
    - How long does it take, and how long in the worst case?
    - Ask the room: who measures worst case rather than average? What do you measure with?

---

<div class="tag">Why</div>

## Every callback has a deadline.<br><span class="then">One of them runs the model.</span>

<div class="deadline">
  <div class="deadline-budget"><span>budget per callback</span></div>
  <div class="deadline-bars"><i style="--i:0"></i><i style="--i:1"></i><i style="--i:2"></i><i style="--i:3"></i><i style="--i:4"></i><i style="--i:5"></i><i style="--i:6"></i><i style="--i:7"></i><i style="--i:8"></i><i style="--i:9"></i><i style="--i:10"></i><i style="--i:11"></i><i style="--i:12"></i><i style="--i:13"></i><i style="--i:14"></i><i style="--i:15"></i><i style="--i:16"></i><i style="--i:17"></i><i style="--i:18"></i><i style="--i:19"></i><i style="--i:20"></i><i style="--i:21"></i><i style="--i:22"></i><i style="--i:23"></i><i style="--i:24"></i><i style="--i:25"></i><i style="--i:26"></i><i style="--i:27"></i><i style="--i:28"></i><i style="--i:29"></i><i style="--i:30"></i><i class="runs" style="--i:31"><span>375 %</span></i></div>
  <div class="deadline-axis">32 callbacks of 64 samples</div>
</div>

Note:
    - budget = host block size / sample rate: 64 samples at 48 kHz are 1.33 ms; 512 are 10.7 ms; 2048 are 42.7 ms.
    - The model takes 2048 samples, 42.7 ms of audio. The host hands us 64 samples, 1.33 ms of time.
    - 31 callbacks do nothing. The 32nd runs the whole forward pass, and still has 1.33 ms.
    - This is the slide to linger on: the audio duration of the model block and the deadline of a callback are unrelated numbers.

---

<div class="tag">Why</div>

## 12 % on average.<br><span class="then">375 % in the callback that runs it.</span>

<div class="meters">
  <div class="meter">
    <div class="meter-bar"><i style="--load:12%"></i></div>
    <div class="meter-label">average load<strong>12 %</strong></div>
  </div>
  <div class="meter over">
    <div class="meter-bar"><i style="--load:100%"></i></div>
    <div class="meter-label">the callback that runs the model<strong>375 %</strong></div>
  </div>
</div>

Note:
    - Forward pass on this laptop: about 5 ms.
    - Average: 5 ms of work per 42.7 ms of audio, 12 %. The callback that runs it: 5 ms in a 1.33 ms budget, 375 %.
    - A CPU meter showing 12 % and an audio stream that clicks every 43 ms.

---

<div class="tag">Goal</div>

## How long does one call take?<br><span class="then">Typically, and at worst.</span>

<div class="statement-points">
  <div><small>1</small>Run the model inside the timed loop</div>
  <div><small>2</small>Implement <code>percentile()</code> for p95 and p99</div>
  <div><small>3</small>Time one call per repetition</div>
</div>

---

<div class="tag">What's given</div>

## 1 new library, 1 file to edit.

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">models/ · common/assets/</div>
    {{FILE:forward_stateful.pt}}
    {{FILE:test_signal.h}}
  </div>
  <div class="given-column">
    <div class="given-label">Helpers</div>
    <div class="given-folder">common/helpers/ · CMake</div>
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
    - main.cpp: three TODOs, k_repetitions = 200.

---

<div class="tag">Helpers · Google Benchmark</div>

## Only the loop is timed.

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

---

## Task

<div class="task-timer" data-minutes="5"></div>

Fill in the TODOs in `steps/03_benchmark/exercise/main.cpp`

```bash
cmake --build --preset release --target step03_exercise
./build/bin/step03_exercise
```

<div class="run">
  <div class="run-step" data-seconds="2.6">
    <div class="run-title"><span>1</span>The timed loop</div>
    <div class="run-visual run-timed">
      <div class="stopwatch"><i></i></div>
      <div class="mini-model">process()</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>2</span><code>percentile()</code></div>
    <div class="run-visual run-sort">
      <div class="sort-bars"><i style="--h:30%;--from:0;--to:1"></i><i style="--h:70%;--from:1;--to:6"></i><i style="--h:45%;--from:2;--to:3"></i><i style="--h:90%;--from:3;--to:7"></i><i style="--h:20%;--from:4;--to:0"></i><i style="--h:55%;--from:5;--to:4"></i><i style="--h:40%;--from:6;--to:2"></i><i style="--h:65%;--from:7;--to:5"></i></div>
      <div class="sort-mark">p95</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>3</span>One call per repetition</div>
    <div class="run-visual run-hist">
      <div class="hist-bars"><i style="--h:12%;--i:0"></i><i style="--h:38%;--i:1"></i><i style="--h:82%;--i:2"></i><i style="--h:100%;--i:3"></i><i style="--h:58%;--i:4"></i><i style="--h:26%;--i:5"></i><i style="--h:10%;--i:6"></i><i style="--h:0%;--i:7"></i><i style="--h:0%;--i:8"></i><i style="--h:9%;--i:9"></i></div>
      <div class="hist-max">max</div>
      <div class="run-ok">✓ the worst case, measured</div>
    </div>
  </div>
</div>

---

<div class="tag">Solution</div>

## TODO 1 · The timed loop

```cpp
for (auto _ : state) {
    engine().process(block.data(), block_size);
    benchmark::DoNotOptimize(block.data());
}
```

Note:
    - Only the call itself: copying, allocating or printing in here is measured too.

---

<div class="tag">Solution</div>

## TODO 2 · <code>percentile()</code>

```cpp
std::vector<double> sorted(times);
std::sort(sorted.begin(), sorted.end());
const auto index = static_cast<size_t>(fraction * (sorted.size() - 1) + 0.5);
return sorted[index];
```

Note:
    - Sort a copy, and take the entry `fraction` of the way through it.

---

<div class="tag">Solution</div>

## TODO 3 · One call per repetition

```cpp
BENCHMARK(forward_pass)
    ->Iterations(1)
    ->Repetitions(k_repetitions)
    // ...
```

Note:
    - Left alone, Google Benchmark runs the body as often as it likes and reports the average: the one number that cannot show a worst case.

---

<div class="tag">Done</div>

## You know your worst case.<br><span class="then">Now it has to fit the budget.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
    - Run it three times and watch which numbers move. The mean is stable across runs; the max is not, and it is the one that decides.
    - mean / median: what a CPU meter shows. p95 / p99: what most users hit sometimes. max: what decides whether you shipped a broken plugin.
    - Page faults, the allocator, a thermal step, the scheduler. The worst case has to fit the budget, with room to spare: everything else in the host is fighting for that deadline too.
    - Measured on an ordinary thread, with nothing competing. In a callback, allocations, locks and page faults are not slow but fatal: step 5.
