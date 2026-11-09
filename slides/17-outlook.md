<h1>Outlook</h1>

v3: inline execution, GPU, NPU, zero copy

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- We have reached the outlook.
- We covered a lot today. But there is even more to come

- anira has a few more upcoming features that were not part of this workshop. Here they are, so you know what is coming.

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

<div class="cards-note">host blocks of exactly 512 · declared 1 to 512, as in step 9: 5295</div>

<div class="lat-why">Run it on the audio thread, inside <code>process</code>: does anira support that?</div>

Note:
- First: we want to lower the latency further.
- A big part of the latency is the inference, because the model runs on another thread.
- If the engine is real-time safe and fast, we could skip that trip and run the inference directly on the audio thread.

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
    <div class="deadline-budget" style="bottom: calc(40px + (100% - 40px) * 0.97)"><span>host block 2048 · 46.4 ms · worst case 18 ms ✓</span></div>
    <div class="deadline-bars"><i class="runs" style="--i:0;height:33.5%"><span>forward pass<br>13 ms ✓</span></i></div>
    <div class="deadline-axis"><span></span><span>every callback · runs the model</span></div>
  </div>
</div>

Note:
- Back to part 1, step 4.
- [click] With host blocks of 512, a callback lasts 11.6 ms, but the model needs 13 ms. Too late.
- [click] With host blocks of 2048, every callback runs the model: 13 ms in 46.4 ms. It fits, with plenty of headroom.
- In that case it makes sense to run the model on the audio thread.

---

<div class="tag">v3 · Preview</div>

## Inline execution.<br><span class="then">No inference term in the latency.</span>

<div class="inline-tree">
  <div class="it-checks">
    <div class="it-check fragment" data-fragment-index="1"><span class="it-box">✓</span><div><b>real-time safe</b><small>the engine declares it: <code>REALTIME_SAFE</code> · none of the built-in engines does, your own can</small></div></div>
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

<div class="cards-note">Planned contract key: <code>execution: worker | inline | auto</code>, fixed at <code>prepare</code></div>

<div class="cards-note">Not yet: it comes with the final v3 release, together with GPU and NPU accelerators. Until then every inference runs on the pool.</div>

Note:
- That option comes with the final v3: inline execution.
- [click] First condition: you declare the engine real-time safe, the flag REALTIME_SAFE.
- [click] Second: it fits into the callback. prepare measures it, within inline_ratio of the block period.
- [click] Our example, host blocks of 2048: nothing to adapt on either side.
- Both conditions met: the audio thread runs it, and the latency is only the model's own, 2224.
- Otherwise the pool runs it, as today: 4272.
- Almost half the latency. Depending on your use case, that can matter a lot.
- But inline only works when the host block is long enough for the model.
- So prepare checks it every time and picks inline or the pool.
- In a plugin it's unlikely: the host can send other block sizes than it announced in prepare.
- In the contract you set this with execution: worker, inline or auto.

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
- Next: memory beyond the CPU.
- Today every tensor lives in host memory.
- With v3 a tensor says where it lives: CPU, GPU or NPU. The groundwork is done.
- Missing are the back ends: CUDA, Metal, Vulkan or WebGPU, and NPUs through LiteRT and ExecuTorch.
- Then inference runs faster, and data already on the GPU, like a camera image, goes in without a copy.
- The hard part: moving data between APIs, say CUDA to WebGPU and back, costs copies.
- For that we build the engines with provider support, and the back end gets a provider field.
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
- With that field, every back end is a path: hardware, provider, engine.
- [click] Which path is fastest? We don't know yet.
- After v3.0 we will measure them in our research, for audio and video.
- [click] For audio, the CPU paths meet real time today. The GPU is barely studied.
- [click] If you have run audio on a GPU, talk to us during the conference.
- We will post the paper on our channels.

---

<div class="tag">Take away</div>

## Same API.<br><span class="then">Another memory.</span>

<div class="statement-points">
  <div>The configs and the three calls stay</div>
  <div>A plan gains a provider; the tensor gains a location</div>
  <div>The real-time rules do not change: the pool runs the model, inline only an engine that declares itself real-time safe</div>
</div>

<div class="cards-note">Start today on v3.0.0-alpha.3: the calls stay, details may still move until 3.0, the changelog says how.</div>

Note:
- All of this comes with the same API as today.
- The configs and the three calls stay as you wrote them.
- So you do not have to wait: start your implementation with the alpha tag from this workshop.
- When the new features land, even the GPU, you just update anira.

---

## Thank you.<br><span class="then">Any questions?</span>

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
- Thank you very much for attending this workshop.
- The two QR codes: the workshop repository, with every solution, and anira.
- We would love your feedback on this workshop. Find us during the next three days.
- [pause] Any questions?
- Enjoy the conference.
