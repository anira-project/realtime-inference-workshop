<h1>Step 7<br>Latency and the<br>dry/wet mix</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
    - The output is late. By how much, exactly?
    - Ask first: who reports latency in their plugins today, and where did the number come from?

---

<div class="tag">Why</div>

## Fixed beats small.

<div class="align">
  <div class="align-row">
    <div class="align-label">changes with load</div>
    <div class="align-track">
      <div class="align-wave dry">{{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=100|from=2|to=3}}</div>
      <div class="align-wave wet jitter">{{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=100|from=2|to=3}}</div>
    </div>
  </div>
  <div class="align-row">
    <div class="align-label">fixed</div>
    <div class="align-track">
      <div class="align-wave dry">{{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=100|from=2|to=3}}</div>
      <div class="align-wave wet fixed">{{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=100|from=2|to=3}}</div>
    </div>
  </div>
</div>

Note:
    - A latency that changes with CPU load is not a latency, it is a guess.
    - The host gets one number, once, and lines the track up with it. Everything else in the session is aligned to it.
    - If it changes mid-stream, you get a click at best. So: pick the number the design can always hold, and pad to it.

---

<div class="tag">Why</div>

## Dry and wet have to meet.

<div class="align meet">
  <div class="align-row">
    <div class="align-label">dry</div>
    <div class="align-track">
      <div class="align-wave dry delayed">{{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=100|from=2|to=3}}</div>
    </div>
  </div>
  <div class="align-row">
    <div class="align-label">wet</div>
    <div class="align-track">
      <div class="align-wave wet fixed">{{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=100|from=2|to=3}}</div>
    </div>
  </div>
</div>

Note:
    - The wet path is late by the design: collect a block, hand it to the worker, pick the result up later.
    - Mix an undelayed dry with a late wet and you do not get a blend: you get a slap-back, and comb filtering on the way there.
    - So the dry path is delayed by exactly the same amount.

---

<div class="tag">Goal</div>

## One latency number.<br><span class="then">Dry and wet line up.</span>

<div class="statement-points">
  <div><small>1</small><code>latency_samples()</code>: told to the host, primed, delaying dry</div>
  <div><small>2</small>The mix: both sides equally late, so a crossfade</div>
</div>

---

<div class="tag">What's given</div>

## Step 6, plus a dry path.

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">models/ · common/assets/</div>
    {{FILE:forward_stateful.pt}}
    {{FILE:test_signal.h}}
    {{FILE:target_signal.h}}
  </div>
  <div class="given-column many">
    <div class="given-label">Helpers</div>
    <div class="given-folder">common/helpers/ · CMake</div>
    {{FILE:libtorch_engine.h}}
    {{FILE:ring_buffer.h}}
    {{FILE:host.h}}
    {{FILE:readerwriterqueue.h}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">07_latency/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
    - LatencyProcessor is step 6's processor plus a dry path: a third ring buffer, primed with latency_samples() of silence in prepare(), and set_mix().
    - The host runs in real time, 512 samples per call.

---

## Task

<div class="task-timer" data-minutes="5"></div>

Fill in the TODOs in `steps/07_latency/exercise/main.cpp`

```bash
cmake --build --preset release --target step07_exercise
./build/bin/step07_exercise
```

<div class="run">
  <div class="run-step" data-seconds="2.6">
    <div class="run-title"><span>1</span>Work out the latency</div>
    <div class="run-visual run-latency">
      <div class="lat-part">collect a block</div>
      <div class="lat-part">hand it over</div>
      <div class="run-size">? samples</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3.6">
    <div class="run-title"><span>2</span>Mix dry and wet</div>
    <div class="run-visual run-wave run-mix">
      <div class="mix-wave dry">{{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=120|from=2|to=3}}</div>
      <div class="mix-wave wet">{{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=120|from=2|to=3}}</div>
      <div class="mix-fader"><i></i></div>
      <div class="run-ok">✓ dry and wet line up</div>
    </div>
  </div>
</div>

---

<div class="tag">Solution</div>

## TODO 1 · Work out the latency

```cpp
size_t latency_samples() const { return 2 * k_model.m_input_size; }   // 4096
```

<div class="lat-sum">
  <div class="lat-part">collect a block<strong>2048</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">hand it over<strong>2048</strong></div>
  <div class="lat-plus">=</div>
  <div class="lat-part total">latency<strong>4096</strong></div>
</div>

Note:
    - 4096 samples = 85.3 ms at 48 kHz.
    - The second block is a choice: it buys the worker a full block of time, and in exchange the latency is a fixed number.
    - In a plugin: setLatencySamples(processor.latency_samples()) once in prepareToPlay; the host compensates every other track. Step 8.

---

<div class="tag">Solution</div>

## TODO 2 · Mix dry and wet

```cpp
std::fill_n(samples, num_samples, 0.0f);

if (m_dry.available() >= num_samples) {
    m_dry.pop(m_dry_block.data(), num_samples);
    for (size_t i = 0; i < num_samples; ++i) { samples[i] += (1.0f - m_mix) * m_dry_block[i]; }
}

if (m_output.available() >= num_samples) {
    m_output.pop(m_wet_block.data(), num_samples);
    for (size_t i = 0; i < num_samples; ++i) { samples[i] += m_mix * m_wet_block[i]; }
}
```

Note:
    - If the wet side has nothing ready, the dry side still has to come through.
    - The check: 100 % dry has to come out at exactly 0.0, it is the input, only later. One sample off and this jumps. A wrong latency cannot pass.

---

<div class="tag">Done</div>

## A fixed latency.<br><span class="then">The host can line it up.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
    - Correct, real-time safe, late by a known amount: ring buffers, a worker thread, two queues, a prefill rule, a latency, a dry path to match.
    - Next: all of it in a plugin.
