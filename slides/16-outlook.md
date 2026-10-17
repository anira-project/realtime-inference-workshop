<h1>Outlook</h1>

v3: GPU, NPU, zero copy

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - What the v3 release adds to what you used today. Same API, other memory.

---

<div class="tag">Where the memory is</div>

## A tensor says where it lives.

<div class="cards">
  <div class="card accent">{{ICON:chip}}<div class="card-title">CPU</div><div class="card-text">today: host memory, every engine</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:chip}}<div class="card-title">GPU</div><div class="card-text">CUDA, Metal, Vulkan, GL</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:chip}}<div class="card-title">NPU</div><div class="card-text">LiteRT and ExecuTorch delegates</div></div>
</div>

<div class="cards-note">The provider is the engine's twin: one plan per engine and provider, picked at prepare.</div>

Note:
    - The v3 tensor is a C struct that describes host, device and pinned memory, with a sync token for the device's queue. The engines take the provider of the plan: an ONNX Runtime execution provider, a LiteRT accelerator, an ExecuTorch delegate.
    - The platform bindings are drafts on the v3 branch; the CPU path is what shipped to you today.

---

<div class="tag">Zero copy</div>

## The frame stays where it is.

<div class="cards">
  <div class="card">{{ICON:people}}<div class="card-title">camera</div><div class="card-text">a GPU texture</div></div>
  <div class="card-arrow fragment strike">copy</div>
  <div class="card">{{ICON:model}}<div class="card-title">hand tracking</div><div class="card-text">on the GPU</div></div>
  <div class="card-arrow">→</div>
  <div class="card accent">{{ICON:sliders}}<div class="card-title">8 latents</div><div class="card-text">32 bytes</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:model}}<div class="card-title">decoder</div><div class="card-text">on the CPU, as today</div></div>
</div>

Note:
    - Step 11 copies every frame camera → CPU → engine. With a GPU plan the texture is the tensor: the engine reads it in place, and only the latents cross to the CPU.
    - Same model file, same contract, same three calls. What changes is the plan the handler picks.

---

<div class="tag">Take away</div>

## Same API.<br><span class="then">Another memory.</span>

<div class="statement-points">
  <div>The configs and the three calls stay</div>
  <div>A plan gains a provider; the tensor gains a location</div>
  <div>The real-time rules do not change: the pool runs it, the audio thread moves samples</div>
</div>

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
    - Questions, and the jam.
