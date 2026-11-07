<h1>Step 7<br>Latency and the<br>dry/wet mix</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

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

<div class="tag">The latency</div>

## The latency<br><span class="then">And the model's own delay.</span>

<div class="lat-add with-code">
  <div class="lat-term fragment"><span>buffer adaptation</span><small>2048 − 512</small><strong>1536</strong></div>
  <div class="lat-op fragment">+</div>
  <div class="lat-term fragment"><span>the worker's time</span><small>18 ms → 2 callbacks</small><strong>1024</strong></div>
  <div class="lat-op fragment">+</div>
  <div class="lat-term fragment"><span>the model itself</span><small>decoder 1696 + PQMF 528</small><strong>2224</strong></div>
  <div class="lat-op fragment">=</div>
  <div class="lat-term total fragment"><span>latency</span><small>samples</small><strong>4784</strong></div>
</div>

<div class="fragment">

```cpp
// in prepare(): step 6's prefill, given
adaptation = (k_model.m_input_size - max_block_size);
m_prefill = adaptation + worker_time;
m_latency = m_prefill + k_model.m_latency;  // the number the host is told
```

</div>

Note:
- Buffer adaptation, step 3: the model needs a whole block
- The worker's time, step 6: your worst case from step 4, in whole callbacks
- Together, the prefill: that is how late the host hears the model's stream
- New: the model itself answers late. 2224 samples: 1696 from the decoder's transposed convolutions (with the strides), 528 from the PQMF filter bank. From the export script, not visible from C++
- Up to now it did not matter: we compared against the reference, which has it too. For the dry/wet mix it does: the dry signal has no such delay
- So the latency is our prefill from step 6 plus `k_model.m_latency`
- Total: 4784 samples, about 108 ms at 44.1 kHz. We report this to the host: `setLatencySamples()` in JUCE
- Only for 512: other block sizes give other numbers, part 2
- Honest: fine for mixing, can't play through it live. Part 2 comes back to it

---

<div class="tag">The dry path</div>

## Delay the dry signal<br><span class="then">By the whole latency.</span>

```cpp
// in prepare(): the wet path has step 6's prefill, the dry one gets its own
const size_t dry_prefill = m_latency;
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
