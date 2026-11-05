<h1>Step 7<br>Latency and the<br>dry/wet mix</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- Ask: who reports latency in their plugins? Where does the number come from?
- Our output is late: by how much, and what do we do about it?
- Also the dry/wet mix: common use case, has implications for latency

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
      <div class="align-wave wet jitter">{{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=100|from=2|to=3}}</div>
    </div>
  </div>
  <div class="align-row">
    <div class="align-label">fixed</div>
    <div class="align-track">
      <div class="align-wave dry">{{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=100|from=2|to=3}}</div>
      <div class="align-wave wet fixed">{{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=100|from=2|to=3}}</div>
    </div>
  </div>
</div>

Note:
- What kind of latency do we want? Not the smallest, a **fixed** one
- Picture is stylised: same sine in two colours, real wet is much quieter
- Top: changes with CPU load. Bottom: always the same
- Host asks once, aligns the whole session to that number
- If it changes: track out of time, at best a click
- A latency that changes with load is a guess
- Pick the number the design can always hold, pad up to it

**Script:**

"Before we calculate anything: what kind of latency do we want? You might think as small as possible. But more important is that it's **fixed/predictable**. Here you see two cases, and the picture is stylised: it's the same sine in two colours, the real wet signal is much quieter. At the top the latency changes with the CPU load, at the bottom it's always the same.
The host asks our plugin for its latency once, and then aligns the whole session to that number. If our latency changes while it's running, our track is suddenly out of time, and at best we get a click. A latency that changes with load isn't really a latency, it's a guess. So we pick the number our design can always hold, and we pad up to it."

---

<div class="tag">Motivation</div>

## Dry and wet align<br><span class="then">Now we can crossfade.</span>

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
      <div class="align-wave wet fixed">{{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=100|from=2|to=3}}</div>
    </div>
  </div>
</div>

Note:
- Wet is late by design: collect a block, give the worker its time, and the model answers late
- Undelayed dry + late wet: no blend, a slap-back echo and comb filtering
- Delay dry by exactly the same amount
- Then they line up and we can crossfade

**Script:**

"The second thing is the dry/wet mix. Our wet signal is late because of how we built it: we collect a block, give the worker its time, and the model itself also answers late.
If we mix a dry signal that isn't delayed with a wet signal that is, we don't get a blend. We get a slap-back echo, and comb filtering on the way there. So the dry signal has to be delayed by exactly the same amount as the wet one. Then the two line up and we can crossfade between them."

---

<div class="tag">Goal</div>

## The goal<br><span class="then">One latency number, dry and wet line up.</span>

<div class="statement-points">
  <div><small>1</small>Work out the latency and pass it to the host</div>
  <div><small>2</small>Delay the dry signal so both sides align</div>
  <div><small>3</small>Crossfade dry and wet</div>
</div>

Note:
- Goal: a fixed latency number, wet and dry aligned
- Define the latency
- Tell the host, use it to delay the dry path
- The mix is given: easy once both sides are equally late

**Script:**

"So our goal is: work out a fixed latency number, so that we have a predictable delay and wet and dry are aligned. First we define the latency. We tell it to the host, and we use it to delay the dry path. The mix itself is given: it's easy once both sides are equally late."

---

<div class="tag">Given</div>

## What we get<br><span class="then">Step 6, plus a dry path.</span>

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
    <div class="given-folder">common/helpers/ · CMake</div>
    {{FILE:onnx_engine.h}}
    {{FILE:ring_buffer.h}}
    {{FILE:host.h}}
    {{FILE:threadsafe_ring_buffer.h}}
    {{FILE:worker_thread.h}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">07_latency/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
- No new files
- Step 6's processor, filled in, plus a dry path
- Third ring buffer, step 3's: only the audio thread uses it, filled with silence in `prepare`, as many samples as our latency
- Host still real time, 512 samples per call
- `k_max_inference_ms`: still your number from step 4, in `steps/common/inference_time.h`

**Script:**

"We don't get any new files. This is step 6's processor, filled in, with one addition: a dry path. That's a third ring buffer, the plain one from step 3, because only the audio thread uses it. It gets filled with silence in `prepare`, as many samples as our latency. The host still runs in real time with 512 samples per call. And your number from step 4 is still in `inference_time.h`."

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="5"></div>

Fill in the TODOs in `steps/07_latency/exercise/main.cpp`

```bash
cmake --build --preset release --target step07_exercise
./build/bin/step07_exercise
```

<div class="run" style="grid-template-columns: 1fr 1fr">
  <div class="run-step" data-seconds="2.6">
    <div class="run-title"><span>1</span>Work out the latency</div>
    <div class="run-visual run-latency">
      <div class="lat-part">buffer adaptation</div>
      <div class="lat-part">the worker's time</div>
      <div class="lat-part">the model itself</div>
      <div class="run-size">? samples</div>
    </div>
  </div>
  <div class="run-step" data-seconds="2.6">
    <div class="run-title"><span>2</span>Delay the dry signal</div>
    <div class="run-visual run-wave run-delay">
      <div class="mix-wave dry">{{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=120|from=2|to=3}}</div>
      <div class="mix-wave wet">{{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=120|from=2|to=3}}</div>
      <div class="run-ok">✓ dry and wet line up</div>
    </div>
  </div>
</div>

Note:
- `main.cpp` in steps/07_latency/exercise
- TODO 1: where does the delay come from? Buffer adaptation, the worker's time, the model
- TODO 2: delay the dry signal by that number
- 5 minutes, don't worry if you don't finish, we solve it together

**Script:**

"Your turn. You'll find the `main.cpp` in `steps/07_latency/exercise`. For the first TODO, think about where the delay comes from. There are three parts: the buffer adaptation from step 3, the time we give the worker from step 6, and the model itself. In the second TODO you delay the dry signal by that number. You get 5 minutes, and again, don't worry if you don't make it, we'll go through it together."

*Timebox.*

---

<div class="tag">Solution</div>

## TODO 1: Work out the latency

<div class="lat-sum" style="margin-bottom: 5rem">
  <div class="lat-part">buffer adaptation<small>2048 − 512</small><strong>1536</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">rounded max inference time<small>15.3 ms → 2 callbacks</small><strong>1024</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">the model itself<small>decoder 1696 + PQMF 528</small><strong>2224</strong></div>
  <div class="lat-plus">=</div>
  <div class="lat-part total">latency<strong>4784</strong></div>
</div>

```cpp
// in prepare(), after step 6's prefill: buffer adaptation + the worker's time
m_latency = m_prefill + k_model.m_latency;  // <- fill out: 4784
```

Note:
- Buffer adaptation: the model needs a whole block, the host brings 512 at a time: 2048 − 512 = 1536 (step 3)
- The worker's time: your worst case from step 4, rounded up to whole callbacks (step 6). A choice
- Smaller: the worker might not be done, latency no longer fixed
- 15.3 ms don't fit in one callback of 11.6 ms, they take two: 1024. Yours may differ
- The model itself: 2224. 1696 from the decoder's transposed convolutions (with the strides), 528 from the PQMF filter bank. From the export script, not visible from C++
- In general a model is late because of what it does inside: filters and filter banks, STFT windows, layers that look ahead. Ours was trained causal, so no look-ahead
- Step 6's prefill already holds the first two: `m_prefill + k_model.m_latency`
- Total: 4784 samples, about 108 ms at 44.1 kHz. We report this to the host: `setLatencySamples()` in JUCE
- Only for 512: other block sizes give other numbers, part 2
- Honest: fine for mixing, can't play through it live. Part 2 comes back to it

**Script:**

"Let's add it up. First the buffer adaptation from step 3: the model needs a whole block before it can run, and the host brings 512 samples at a time, so that's 2048 minus 512, 1536 samples. Second, the time we give the worker: your worst case from step 4, rounded up to whole callbacks, like in step 6. That one is a choice. We could make it smaller, but then the worker might not be done in time, and the latency wouldn't be fixed anymore. My 15.3 milliseconds don't fit into one callback of 11.6 milliseconds, they take two, so that's 1024; yours may differ. And third, the model itself is late by 2224 samples: 1696 from the transposed convolutions in the decoder, and 528 from the PQMF filter bank. We can't see that from C++, it comes from the export script.
The first two are exactly step 6's prefill, so in code it's the prefill plus the model's latency. In total: 4784 samples, that's about 108 milliseconds at 44.1 kHz. In a plugin, this is the number we report to the host, in JUCE with `setLatencySamples`. It only holds for 512, though: other block sizes give other numbers, and that's part 2. And let's be honest: 108 milliseconds is fine for mixing, but you can't play through it live. In part 2 we come back to how low it can go."

---

<div class="tag">Solution</div>

## TODO 2: Delay the dry signal

```cpp
// in prepare(): the wet path gets a prefill of silence, and so does the dry one
m_output.push(silence.data(), m_prefill);       // given: the prefill
m_dry.push(dry_silence.data(), m_latency);      // <- fill out
```

Note:
- Host aligns everything to the number we report
- Prefill the dry ring buffer with `m_latency` of silence before the first callback
- Every sample comes out 4784 samples later, like the wet one
- Wet only gets step 6's prefill, 2560: the model's 2224 are already in what it returns

**Script:**

"The host aligns everything to the number we report. So the dry signal has to start exactly that much later: we prefill the dry ring buffer with `m_latency` samples of silence before the first callback. Then every sample we push in comes out 4784 samples later, just like the wet one. The wet path only gets step 6's prefill, because the model's own 2224 samples are already inside what it returns."

---

<div class="tag">Done</div>

## A fixed latency<br><span class="then">The host can line it up and we can crossfade.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

<div class="cards-note">For a constant 512. anira handles any host block size, smaller blocks than announced included, and computes the latency for it: part 2.</div>

Note:
- Correct, real-time safe, late by a known, fixed amount
- Host can line it up, dry and wet crossfade cleanly
- Only for a constant 512: the plugin in step 8 bypasses outside 512 samples and 44.1 or 48 kHz. Every size: part 2, with anira
- Next: all of this in a JUCE plugin, where the host asks for exactly these numbers

**Script:**

"So now our processor is correct, it's real-time safe, and it's late by a known, fixed amount. The host can line it up with the other tracks, and dry and wet crossfade cleanly. Next, we put all of this into a JUCE plugin, where the host asks for exactly these numbers."
