<h1>Step 7<br>Latency and the<br>dry/wet mix</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

Note:
- There is one more thing we need to talk about before we can create our plug in
- We need to know the latency of the wet path so we can delay the dry signal by the same amount before mixing the two.

---

<div class="tag">The latency</div>

## What is the latency?<br><span class="then">The delay we need to pad the dry signal with.</span>

<div class="lat-add with-code">
  <div class="lat-term fragment"><span>buffer adaptation</span><small>2048 − 512</small><strong>1536</strong></div>
  <div class="lat-op fragment">+</div>
  <div class="lat-term fragment"><span>rounded max inference samples</span><small>18 ms → 2 callbacks</small><strong>1024</strong></div>
  <div class="lat-op fragment">+</div>
  <div class="lat-term fragment"><span>the model itself</span><small>decoder 1696 + PQMF 528</small><strong>2224</strong></div>
  <div class="lat-op fragment">=</div>
  <div class="lat-term total fragment"><span>latency</span><small>samples</small><strong>4784</strong></div>
</div>



Note:
- Ask: Does anyone have an idea how much we have to delay the dry signal by?
- Our wet signal is delayed at least with our prefill from the step before:
   - Buffer adaptation, step 3: the model needs a whole block
   - The rounded max inference samples, step 6: your worst case from step 4, in whole callbacks
- Together: that is how late the host hears the model's stream
- New: the model itself answers late. 2224 samples: 1696 from the decoder's transposed convolutions (with the strides), 528 from the PQMF filter bank. 
- If you want to see this latency implemented we have a solution file for step 7, but I wan't go into detail about it.

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
- A correct set up that is real-time safe, late by a known latency
- Host can line it up, dry and wet crossfade cleanly
