<h1>Step 7<br>Latency and the<br>dry/wet mix</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
    - The output is late. By how much, exactly?
    - Ask first: who reports latency in their plugins today, and where did the number come from?

**Script:**

*Ask the room: "Who reports latency in their plugins today? And where does the number come from?"*
"Our output is late. In this step we'll find out by how much exactly, and what we have to do about it. We will also talk about the dry/wet mix, which is a common use case for audio plugins and has some implications for latency."

---

<div class="tag">Motivation</div>

## Fixed latency over small latency

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

**Script:**

"Before we calculate anything: what kind of latency do we want? You might think as small as possible. But more important is that it's **fixed/predictable**. Here you see two cases. At the top the latency changes with the CPU load, at the bottom it's always the same.
The host asks our plugin for its latency once, and then aligns the whole session to that number. If our latency changes while it's running, our track is suddenly out of time, and at best we get a click. A latency that changes with load isn't really a latency, it's a guess. So we pick the number our design can always hold, and we pad up to it."

---

<div class="tag">Motivation</div>

## Dry and wet align<br><span class="then">The host can crossfade.</span>

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
    - The wet path is late by the design: collect a block, hand it to the worker, pick the result up later. And the model itself answers late.
    - Mix an undelayed dry with a late wet and you do not get a blend: you get a slap-back, and comb filtering on the way there.
    - So the dry path is delayed by exactly the same amount.

**Script:**

"The second thing is the dry/wet mix. Our wet signal is late because of how we built it: we collect a block, hand it to the worker, pick it up later, and the model itself also answers late.
If we mix a dry signal that isn't delayed with a wet signal that is, we don't get a blend. We get a slap-back echo, and comb filtering on the way there. So the dry signal has to be delayed by exactly the same amount as the wet one. Then the two line up and we can crossfade between them."

---

<div class="tag">Goal</div>

## The goal<br><span class="then">One latency number, dry and wet line up.</span>

<div class="statement-points">
  <div><small>1</small>Pass latency to host</div>
  <div><small>2</small>Delay the dry signal so both sides align and crossfade is possible</div>
</div>

Note:
**Script:**

"So our goal is: work out a fixed latency number, so that we have a predictable delay and wet and dry are aligned. First we define the latency.We tell it to the host, and we use it to delay the dry path. And then we write the mix, which is easy once both sides are equally late."

---

<div class="tag">Given</div>

## What we get<br><span class="then">Step 6, plus a dry path.</span>

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">models/ · common/assets/</div>
    {{FILE:model.pt}}
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

**Script:**

"We don't get any new files.This is step 6's processor with one addition: a dry path. That's a third ring buffer, which gets filled with silence in `prepare`, as many samples as our latency. The host still runs in real time with 512 samples per call."

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

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
      <div class="lat-part">the model itself</div>
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

Note:
**Script:**

"Your turn. You'll find the `main.cpp` in `steps/07_latency/exercise`. For the first TODO, think about where the delay comes from. There are three parts: collecting a block, handing it over to the worker, and the model itself. The second TODO is the mix. You get 5 minutes, and again, don't worry if you don't make it, we'll go through it together."

*Timebox.*

---

<div class="tag">Solution</div>

## TODO 1: Work out the latency

<div class="lat-sum fragment" style="margin-bottom: 5rem">
  <div class="lat-part fragment">collect a block<strong>2048</strong></div>
  <div class="lat-plus fragment">+</div>
  <div class="lat-part fragment">hand it over<strong>2048</strong></div>
  <div class="lat-plus fragment">+</div>
  <div class="lat-part fragment">the model itself<strong>2224</strong></div>
  <div class="lat-plus fragment">=</div>
  <div class="lat-part total fragment">latency<strong>6320</strong></div>
</div>

<div class=fragment>

```cpp
size_t latency_samples() const {
    return 2 * k_model.m_input_size + k_model.m_latency;  // 6320
}
```
</div>

Note:
    - 6320 samples = 143.3 ms at 44.1 kHz.
    - The model's 2224 samples (50.4 ms) are its own: causal convolutions and transposed convolutions (1696) plus the filter bank (528). scripts/export_model.py computes it from the export; it is not visible from C++.
    - The wet path is primed with the two blocks only; the model's delay is already inside what it returns. The dry path is delayed by all 6320.
    - The second block is a choice: it buys the worker a full block of time, and in exchange the latency is a fixed number.
    - In a plugin: setLatencySamples(processor.latency_samples()) once in prepareToPlay; the host compensates every other track. Step 8.

**Script:**

"Let's add it up. First, we have to collect a full block before the model can run, that's 2048 samples. Second, the hand-over: we give the worker a full block of time to process, that's another 2048. That one is a choice. We could make it smaller, but then the worker might not be done in time, and the latency wouldn't be fixed anymore. And third, the model itself is late by 2224 samples, because of its convolutions and the filter bank. We can't see that from C++, it comes from the export script.
So in total: 6320 samples, that's about 143 milliseconds at 44.1 kHz. In a plugin, this is the number we report to the host."

---

<div class="tag">Solution</div>

## TODO 2: Mix dry and wet

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

**Script:**

"Now that both sides are equally late, the mix is just a crossfade. We take the dry block times one minus the mix, plus the wet block times the mix. And if the wet side has nothing ready yet, the dry side still comes through."

---

<div class="tag">Done</div>

## A fixed latency<br><span class="then">The host can line it up and cross-fade.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
    - Correct, real-time safe, late by a known amount: ring buffers, a worker thread, two queues, a prefill rule, a latency, a dry path to match.
    - Next: all of it in a plugin.

**Script:**

"So now our processor is correct, it's real-time safe, and it's late by a known, fixed amount. The host can line it up with the other tracks, and dry and wet crossfade cleanly. The last step is to put all of this where a host can actually reach it: in a plugin."
