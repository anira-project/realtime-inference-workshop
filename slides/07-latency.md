<h1>Step 7<br>Latency and the<br>dry/wet mix</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- The host hears it exactly the prefill late
- So how do we line up a dry signal with it?
- A latency that changes with load is a guess
- Pick a fixed number the design can always hold, pad up to it
- Aso we can mix our dry and our wet signal

---

<div class="tag">Motivation</div>

## Fixed latency<br><span class="then">Dry and wet align.</span>

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

---

<div class="tag">Goal</div>

## The goal<br><span class="then">One latency number, dry and wet line up.</span>

<div class="statement-points centered-two">
  <div><small>1</small>Work out the latency and pass it to the host</div>
  <div><small>2</small>Delay the dry signal so both sides align</div>
</div>

Note:
- Goal: a fixed latency number, wet and dry aligned
- Define the latency
- Tell the host, use it to delay the dry path
- The mix is given: easy once both sides are equally late

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
      <div class="lat-plus">latency =</div>
      <div class="lat-part">?</div>
      <div class="lat-plus">+</div>
      <div class="lat-part">?</div>
      <div class="lat-plus">+</div>
      <div class="lat-part">?</div>
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
- TODO 1: where does the delay come from? Everything between a sample going in and coming out
- TODO 2: delay the dry signal by that number
- 5 minutes, don't worry if you don't finish, we solve it together

---

<div class="tag">Solution</div>

## TODO 1: Work out the latency

<div class="lat-add with-code">
  <div class="lat-term fragment"><span>buffer adaptation</span><small>2048 − 512</small><strong>1536</strong></div>
  <div class="lat-op fragment">+</div>
  <div class="lat-term fragment"><span>rounded max inference time</span><small>18 ms → 2 callbacks</small><strong>1024</strong></div>
  <div class="lat-op fragment">+</div>
  <div class="lat-term fragment"><span>the model itself</span><small>decoder 1696 + PQMF 528</small><strong>2224</strong></div>
  <div class="lat-op fragment">=</div>
  <div class="lat-term total fragment"><span>latency</span><small>samples</small><strong>4784</strong></div>
</div>

<div class="fragment">

```cpp
// in prepare(): step 6's prefill, given
adaptation = (k_model.m_input_size - max_block_size);
m_prefill = adaptation + k_max_inference_samples_rounded;
m_latency = m_prefill + k_model.m_latency;  // <- fill out
```

</div>

Note:
- Buffer adaptation: the model needs a whole block, 
- The rounded max time it takes for one forward pass: your worst case from step 4
- The model itself: 2224. 1696 from the decoder's transposed convolutions (with the strides), 528 from the PQMF filter bank. From the export script, not visible from C++
- So it's our prefill from Step 6' plus `k_model.m_latency`
- Total: 4784 samples, about 108 ms at 44.1 kHz. We report this to the host: `setLatencySamples()` in JUCE
- Only for 512: other block sizes give other numbers, part 2
- Honest: fine for mixing, can't play through it live. Part 2 comes back to it

---

<div class="tag">Solution</div>

## TODO 2: Delay the dry signal

```cpp
// in prepare(): the wet path has step 6's prefill, the dry one gets its own
const size_t dry_prefill = m_latency;  // <- fill out
for (size_t i = 0; i < dry_prefill; ++i) {
    const bool pushed = m_dry.try_push(0.0f);
    assert(pushed);  // never fails: m_dry has room for the delay
}
```

Note:
- The wet path gets step 6's prefill (buffer adaptation + the worker's time). The model's own 2224 are already inside what it returns
- The dry path gets the whole latency, `m_latency`: prefill + the model's 2224
- Every sample comes out 4784 samples later, like the wet one

---

<div class="tag">Done</div>

## A fixed latency<br><span class="then">The host can line it up and we can crossfade.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

<div class="cards-note corner-note">For a constant 512. anira handles any host block size, smaller blocks than announced included, and computes the latency for it: part 2.</div>

Note:
- Correct, real-time safe, late by a known, fixed amount
- Host can line it up, dry and wet crossfade cleanly
- Only for a constant 512: the plugin in step 8 bypasses outside 512 samples and 44.1 or 48 kHz. Every size: part 2, with anira
