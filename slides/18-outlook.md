<h1>Outlook</h1>

v3: inline execution, GPU, NPU, zero copy

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- Outlook: inline execution, GPU, NPU and zero copy in v3.
- Today you ran the v3 pre-release on the CPU.
- What follows is the rest of v3: the same API, with the model on the audio thread or the memory somewhere else.

---

<div class="tag">Latency</div>

## A big part is the inference.<br><span class="then">What if the engine is fast and real-time safe?</span>

<div class="lat-sum">
  <div class="lat-part">block adaptation<strong>1536</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part focus">inference<small>on the pool</small><strong>1024</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">the model itself<strong>2224</strong></div>
  <div class="lat-plus">=</div>
  <div class="lat-part total">latency<strong>4784</strong></div>
</div>

<div class="lat-why">Run it on the audio thread, inside <code>process</code>: does anira support that?</div>

Note:
- Remember the latency anira computed at prepare: 1536 for the block adaptation, 1024 for the inference, and the model's own 2224.
- A big part of it is the inference: the trip to the pool costs whole host blocks, here two.
- For a tiny model that takes a fraction of a callback, that trip costs more than the model itself.
- So what if the engine is fast and declares itself real-time safe? Then it could run on the audio thread, inside process, and that term would be gone.
- Does anira support that?

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 4</div>

## Inline, the callback runs the model.<br><span class="then">Does it fit into one callback?</span>

<div class="deadline-pair">
  <div class="deadline fragment" data-fragment-index="1">
    <div class="deadline-budget"><span>host block 512 · 11.6 ms</span></div>
    <div class="deadline-bars"><i style="--i:0"></i><i style="--i:1"></i><i style="--i:2"></i><i class="runs" style="--i:3;height:33.5%"><span>forward pass<br>13 ms ✗</span></i></div>
    <div class="deadline-axis"><span>callbacks 1–3 · only copy</span><span>callback 4 · runs the model</span></div>
  </div>
  <div class="deadline fits fragment" data-fragment-index="2">
    <div class="deadline-budget" style="bottom: calc(40px + (100% - 40px) * 0.97)"><span>host block 2048 · 46.4 ms</span></div>
    <div class="deadline-bars"><i class="runs" style="--i:0;height:33.5%"><span>forward pass<br>13 ms ✓</span></i></div>
    <div class="deadline-axis"><span></span><span>every callback · runs the model</span></div>
  </div>
</div>

Note:
- Inline means the audio callback runs the model itself. Remember the picture from step 4.
- [click] With host blocks of 512, three callbacks only copy samples, and the fourth runs the whole forward pass: 13 ms in a callback of 11.6 ms. Too late, every time.
- [click] With host blocks of 2048, the host block is the model block: every callback runs the model, and it has 46.4 ms for 13. That fits.
- So inline needs the measured time to fit into one callback, and with blocks of 512 the callback that completes a model block cannot do that. The other condition is one step 5 showed by hand: the engine has to be real-time safe.

---

<div class="tag">v3 · Preview</div>

## Inline execution.<br><span class="then">No inference term in the latency.</span>

<div class="inline-tree">
  <div class="it-checks">
    <div class="it-check fragment" data-fragment-index="1"><span class="it-box">✓</span><div><b>real-time safe</b><small>the engine declares it: <code>REALTIME_SAFE</code></small></div></div>
    <div class="it-check fragment" data-fragment-index="2"><span class="it-box">✓</span><div><b>fits into the callback</b><small>measured at <code>prepare</code>, within <code>inline_ratio</code> of the block period</small></div></div>
  </div>
  <div class="it-example fragment" data-fragment-index="3">Host blocks of 2048, the model's block size: nothing to adapt</div>
  <div class="it-branches fragment" data-fragment-index="3">
    <div class="it-branch yes">
      <div class="it-label">both ✓</div>
      <div class="it-thread">audio thread · <code>inline</code></div>
      <div class="lat-sum it-sum"><div class="lat-part">adaptation<small>2048 − 2048</small><strong>0</strong></div><div class="lat-plus">+</div><div class="lat-part focus">inference<strong>0</strong></div><div class="lat-plus">+</div><div class="lat-part">model<strong>2224</strong></div><div class="lat-plus">=</div><div class="lat-part total">latency<strong>2224</strong></div></div>
    </div>
    <div class="it-branch no">
      <div class="it-label">any ✗</div>
      <div class="it-thread">inference thread · the pool</div>
      <div class="lat-sum it-sum"><div class="lat-part">adaptation<small>2048 − 2048</small><strong>0</strong></div><div class="lat-plus">+</div><div class="lat-part">inference<strong>2048</strong></div><div class="lat-plus">+</div><div class="lat-part">model<strong>2224</strong></div><div class="lat-plus">=</div><div class="lat-part total">latency<strong>4272</strong></div></div>
    </div>
  </div>
</div>

<div class="cards-note">Not yet: it comes with the final v3 release, together with GPU and NPU accelerators. Until then every inference runs on the pool.</div>

Note:
- Not yet, but with the final v3 release: inline execution, the model inside process, on the audio thread.
- [click] First check: the engine declares itself real-time safe, no allocation, no lock, no syscall in its run: the flag REALTIME_SAFE.
- [click] Second: it fits into the callback on this device. prepare measures it anyway, and it may take at most inline_ratio of the block period.
- [click] Our example with host blocks of 2048, the model's own block size. That is why the adaptation is 0 on both sides: 2048 − gcd(2048, 2048). With blocks of 512 it would be 1536, the same on both sides, and our model would not fit into one callback, as the previous slide showed.
- Both checks: the audio thread runs it inline, and the latency is only the model's own, 2224.
- Either missing: the inference thread runs it, as today. With the same blocks of 2048 that is 4272, one host block for the inference on top; computed by anira at prepare.
- In the contract this is execution: worker, inline or auto, per plan, fixed at prepare; auto runs the two checks, and an inline that cannot fit is refused at prepare.
- It comes together with GPU and NPU accelerators. Until then every inference runs on the pool, as today.

---

<div class="tag">Where the memory is</div>

## A tensor says where it lives.<br><span class="then">CPU today. GPU and NPU follow.</span>

<div class="cards">
  <div class="card accent">{{ICON:chip}}<div class="card-title">CPU</div><div class="card-text">today: host memory, every engine</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:chip}}<div class="card-title">GPU</div><div class="card-text">later: CUDA, Metal, Vulkan, GL</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:chip}}<div class="card-title">NPU</div><div class="card-text">later: LiteRT and ExecuTorch delegates</div></div>
</div>

<div class="cards-note">The provider is the engine's twin: one plan per engine and provider, picked at prepare.</div>

Note:
- Today anira runs on the CPU: every tensor lives in host memory, on every engine. That is what you ran today.
- In v3 a tensor also says where it lives: on the CPU, the GPU or an NPU.
- GPU and NPU support follows: on the GPU through CUDA, Metal, Vulkan or GL, on an NPU through the hardware back ends of LiteRT and ExecuTorch.
- The provider is chosen with the engine: one plan per engine and provider, picked at prepare.
---

<div class="tag">After v3.0</div>

## Which combination is fastest?<br><span class="then">After v3.0, we benchmark them systematically.</span>

<div class="combos" data-chains="0,2,0;0,3,2;1,1,1;4,5,3;2,6,0">
  <svg class="combos-svg" aria-hidden="true"><g class="combos-base"></g><g class="combos-hot fragment"></g></svg>
  <div class="combos-col"><div class="combos-head">Hardware</div><div class="combos-node">Apple Silicon</div><div class="combos-node">NVIDIA</div><div class="combos-node">AMD</div><div class="combos-node">Intel</div><div class="combos-node">Qualcomm</div></div>
  <div class="combos-col"><div class="combos-head">Execution provider</div><div class="combos-node">CPU</div><div class="combos-node">CUDA</div><div class="combos-node">CoreML</div><div class="combos-node">Metal</div><div class="combos-node">DirectML</div><div class="combos-node">Vulkan</div><div class="combos-node">WebGPU</div></div>
  <div class="combos-col"><div class="combos-head">Engine</div><div class="combos-node">ONNX Runtime</div><div class="combos-node">LibTorch</div><div class="combos-node">LiteRT</div><div class="combos-node">ExecuTorch</div><div class="combos-node">TFLite</div></div>
</div>

<div class="combos-row fragment">
  <div class="combos-audio"><span>Audio</span><b class="ok">CPU ✓</b><b class="open">GPU ?</b><em>barely studied</em></div>
</div>

<div class="combos-invite fragment">Measured some of these already? We would like to compare notes.</div>

Note:
- The tensor can live anywhere now, and that leaves an open question.
- Hardware, execution provider, engine: which path is fastest for a given model?
- [click] Here are a few of the candidate paths; each one is a question, not an answer.
- [click] For audio, the CPU paths are measured and they meet real time.
- On the GPU it has barely been studied.
- After v3.0 we will benchmark these combinations systematically.
- [click] If you have numbers for any of these, come and talk to us.
- The thread pool came out of one conversation after a talk, and we would like the next idea to come the same way.

---

<div class="tag">Zero copy</div>

## The frame stays where it is.<br><span class="then">Only the latents cross over.</span>

<div class="zc">
  <div class="zc-row">
    <div class="zc-label">With a copy</div>
    <div class="lat-sum zc-sum">
      <div class="lat-part">one 4K frame<small>3840 × 2160 × 4 B</small><strong>33.2 MB</strong></div>
      <div class="lat-plus">×</div>
      <div class="lat-part">frames<small>a second</small><strong>30</strong></div>
      <div class="lat-plus">×</div>
      <div class="lat-part">GPU → CPU → GPU<small>there and back</small><strong>2</strong></div>
      <div class="lat-plus">=</div>
      <div class="lat-part total">moved<strong>≈ 2 GB/s</strong></div>
    </div>
  </div>
  <div class="zc-row fragment">
    <div class="zc-label">Zero copy</div>
    <div class="lat-sum zc-sum">
      <div class="lat-part">8 latents<small>8 × 4 B</small><strong>32 B</strong></div>
      <div class="lat-plus">×</div>
      <div class="lat-part">frames<small>a second</small><strong>30</strong></div>
      <div class="lat-plus">=</div>
      <div class="lat-part total">moved<strong>960 B/s</strong></div>
    </div>
  </div>
</div>

<div class="cards-note">The camera texture is the hand tracker's tensor on the GPU. Only what the decoder on the CPU needs crosses over: about two million times less.</div>

Note:
- Take Step 11 with a 4K camera at 30 frames a second.
- One frame as RGBA, 8 bit: 3840 by 2160 pixels, 4 bytes each, 33 MB.
- Today every frame goes from the camera's GPU memory to the CPU, and for a GPU plan back to the GPU again: twice 33 MB, 30 times a second, about 2 GB every second, before the model has done anything.
- [click] With a GPU plan the texture is the tensor, and the hand tracker reads it in place.
- Only the eight latents cross over to the decoder on the CPU: 32 bytes per frame, under a kilobyte a second.
- That is the overhead zero copy removes: two gigabytes a second of copies, and the time and memory they take on every platform.
- Same model file, same contract, same calls: only the plan changes.

---

<div class="tag">Take away</div>

## Same API.<br><span class="then">Another memory.</span>

<div class="statement-points">
  <div>The configs and the three calls stay</div>
  <div>A plan gains a provider; the tensor gains a location</div>
  <div>The real-time rules do not change: the pool runs the model, inline only an engine that declares itself real-time safe</div>
</div>

Note:
- Same API, another memory.
- The configs and the three calls stay as you wrote them today.
- A plan gains a provider, and a tensor gains a location.
- In part 1 you built every piece by hand; in part 2 they became two files and three calls.
- The real-time rules do not change on any device: the pool runs the model, and only an engine that declares itself real-time safe may run inline on the audio thread.

---

## Thank you.

<div class="qr-row">
  <div class="qr">
    {{QR:HTTPS://GITHUB.COM/ANIRA-PROJECT/REALTIME-INFERENCE-WORKSHOP}}
    <div class="qr-label">Workshop</div>
    <div class="qr-url">github.com/anira-project/realtime-inference-workshop</div>
  </div>
  <div class="qr">
    {{QR:HTTPS://GITHUB.COM/ANIRA-PROJECT/ANIRA}}
    <div class="qr-label">anira</div>
    <div class="qr-url">github.com/anira-project/anira</div>
  </div>
</div>

Note:
- Thank you.
- Both repositories are on the slide: the workshop, with every solution, and anira itself.
- v3.0 ships at the end of November.
- If you measure one of the engine and provider combinations, or you have a model that behaves differently, come and talk to us.
- That is how anira got to where it is.
- Questions?
