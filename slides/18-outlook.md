<h1>Outlook</h1>

v3: GPU, NPU, zero copy

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- Outlook: GPU, NPU and zero copy in v3.
- Today you ran the v3 pre-release on the CPU.
- What follows is the rest of v3: the same API, with the memory somewhere else.

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
- Today every tensor lives in host memory, on every engine.
- In v3 a tensor also says where it lives: on the CPU, the GPU or an NPU.
- On the GPU through CUDA, Metal, Vulkan or GL, on an NPU through the hardware back ends of LiteRT and ExecuTorch.
- The provider is chosen with the engine: one plan per engine and provider, picked at prepare.
- The platform bindings are still drafts on the v3 branch; the CPU path is what you ran today.

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
- Take Step 11: today every camera frame is copied from the camera to the CPU and then to the engine.
- [click] With a GPU plan the texture is the tensor, and the engine reads it in place.
- Only the eight latents, 32 bytes, cross over to the CPU.
- The decoder stays on the CPU, next to the audio thread, as today.
- Same model file, same contract, same calls: only the plan changes.

---

<div class="tag">Take away</div>

## Same API.<br><span class="then">Another memory.</span>

<div class="statement-points">
  <div>The configs and the three calls stay</div>
  <div>A plan gains a provider; the tensor gains a location</div>
  <div>The real-time rules do not change: the pool runs it, the audio thread moves samples</div>
</div>

Note:
- Same API, another memory.
- The configs and the three calls stay as you wrote them today.
- A plan gains a provider, and a tensor gains a location.
- In part 1 you built every piece by hand; in part 2 they became two files and three calls.
- The real-time rules do not change on any device: the pool runs the model, and the audio thread only moves samples.

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
