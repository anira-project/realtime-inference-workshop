<h1>Outlook</h1>

v3: inline execution, GPU, NPU, zero copy

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- We have reached the outlook.
- We covered a lot today. As we said, the final anira v3 comes out at the end of this month.
- It has a few more features that were not part of this workshop. Here they are, so you know what is coming.

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
- Look at the sum anira computed at prepare: 1536 for the block adaptation, 1024 for the inference, 2224 for the model itself.
- A big part comes from moving the inference to a separate thread.
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
- [click] With host blocks of 512, the callback that completes a model block runs the whole forward pass: 13 ms in a callback of 11.6 ms. Too late.
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
- In the contract this is execution: worker, inline or auto, fixed at prepare.

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
- Today every tensor lives in host memory, and that is what you ran today.
- With v3 a tensor says where it lives: CPU, GPU or NPU.
- So you can move the inference to a GPU or an NPU to make it faster, or, if your data is already on the GPU, run the inference there directly.
- GPU through CUDA, Metal, Vulkan or GL; NPU through the hardware back ends of LiteRT and ExecuTorch.
- The provider is picked with the engine: one plan per engine and provider, at prepare.

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
- That gives us many options: hardware, execution provider, engine.
- [click] Which path is fastest? Each line here is a question, not an answer.
- After v3.0 we will measure these combinations in our research, for audio and also for video.
- [click] For audio, most work today runs on the CPU, and those paths meet real time.
- On the GPU it is barely studied; there are a few examples.
- [click] If you have run inference on the GPU already, we would like to hear from you during the conference and compare notes.
- When the paper is out, we will post it on our channels.

---

<div class="tag">Zero copy</div>

## The frame stays where it is.<br><span class="then">Only the landmarks cross over.</span>

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
  <div class="zc-row fragment" data-fragment-index="1">
    <div class="zc-label">Zero copy</div>
    <div class="lat-sum zc-sum">
      <div class="lat-part">landmarks<small>2 hands × 21 × 3 × 4 B</small><strong>504 B</strong></div>
      <div class="lat-plus">×</div>
      <div class="lat-part">frames<small>a second</small><strong>30</strong></div>
      <div class="lat-plus">=</div>
      <div class="lat-part total">moved<strong>≈ 15 KB/s</strong></div>
    </div>
  </div>
</div>

<div class="cards-note fragment" data-fragment-index="1">The camera texture is the hand tracker's tensor on the GPU. Only its landmarks cross over to the CPU: about 130,000 times less.</div>

Note:
- The last feature: zero copy.
- Take step 10 with a 4K camera at 30 frames a second.
- One frame is 3840 by 2160 pixels, 4 bytes each: 33 MB.
- Today every frame is copied from the GPU to the CPU, and for a GPU plan back again: about 2 GB every second, before the model has done anything.
- [click] With zero copy, the texture on the GPU is the tensor. We pass a pointer, nothing is copied.
- Only the landmarks cross over to the CPU, 21 points per hand: about 500 bytes a frame.
- That removes the overhead of the copies: the time and the memory bandwidth.

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
- A plan gains a provider, a tensor gains a location.
- The real-time rules stay: the pool runs the model, and only an engine declared real-time safe runs inline.
- So you do not have to wait: start with v3.0.0-alpha.3, the version from this workshop, right after ADC.

---

<div class="tag">Bonus</div>

## Opening the model.<br><span class="then">Encoder, eight numbers, decoder.</span>

<div class="note-box">A bonus step for after the workshop, in the repository: <code>steps/bonus/</code>, its slides linked from its README.</div>

Note:
- One more step for after the workshop: the bonus.
- We cut the model in the middle: an encoder, eight numbers, a decoder.
- Two handlers on anira, and your code sits between them.
- You have the repository: the exercise is in steps/bonus, and its README links the slides.

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
- Thank you very much for attending this workshop.
- The two QR codes: the workshop repository, with every solution, and anira.
- We would love your feedback on this workshop. Find us during the next three days.
- Enjoy the conference.
