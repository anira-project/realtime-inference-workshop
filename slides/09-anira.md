<h1>anira</h1>

Replacing the hand-built parts

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->
<!-- part: anira -->

Note:
- Welcome back, this is part 2.
- In part 1 you built every piece by hand, for one plugin on one track.
- In part 2 we look at anira, the library that replaces those hand-built parts.

---

<div class="tag">Part 1</div>

## Six things real-time inference needs.<br><span class="then">We solved each one, the simple way.</span>

<div class="needs">
  <div class="card need">{{ICON:model}}<div class="card-title">Inference</div><div class="card-text">steps 1, 2, 3 · one engine, one block size</div></div>
  <div class="card need">{{ICON:gauge}}<div class="card-title">Benchmarking</div><div class="card-text">step 4 · one benchmark, on our machine</div></div>
  <div class="card need">{{ICON:shield}}<div class="card-title">Real-time safety</div><div class="card-text">step 5 · RTSan, run once</div></div>
  <div class="card need">{{ICON:threads}}<div class="card-title">Threading</div><div class="card-text">steps 6, 8 · a worker per instance</div></div>
  <div class="card need">{{ICON:clock}}<div class="card-title">Minimal latency</div><div class="card-text">step 7 · a prefill, counted by hand</div></div>
  <div class="card need">{{ICON:flag}}<div class="card-title">Real-time guarantees</div><div class="card-text">steps 4, 7 · hoping the budget holds</div></div>
  <div class="needs-anira fragment"><span>anira</span> covers all six, for one plugin instance or many.</div>
</div>

Note:
- Here is part 1 in one picture: six things real-time inference needs.
- Inference, benchmarking, real-time safety, threading, minimal latency, and real-time guarantees.
- We solved each one the simple way: one engine, one machine, one plugin instance.
- [click] anira covers all six and provides often improvements over our simple implementations, which we will show soon.
- First, what anira is and where it comes from.

---

<div class="tag">What anira is</div>

## A C++ library for real-time inference.<br><span class="then">One configuration, several engines and platforms.</span>

<div class="pitch">
  <div class="pitch-label config">You configure</div>
  <div class="pitch-label hub">Real-time</div>
  <div class="pitch-label targets">Everywhere</div>
  <div class="pitch-files">{{FILE:model.json}}{{FILE:contract.json}}</div>
  <div class="pitch-arrow first">→</div>
  <div class="pitch-hub">
    <div class="pitch-mark"><img src="assets/images/logos/anira-mark.png" alt="anira">{{ICON:cpp}}</div>
    <div class="pitch-does">threads · buffers · latency · safety</div>
    <div class="pitch-abi"><span class="core">one C ABI</span><span class="arrow">↓</span><div class="pitch-abi-bindings"><span>C++</span><span>TypeScript</span></div></div>
  </div>
  <div class="pitch-arrow second">→</div>
  <div class="pitch-targets">
    <div class="pitch-row fragment">
      <div class="pitch-sublabel">Engines</div>
      <div class="pitch-engines">
        <figure><img src="assets/images/engines/pytorch.svg" alt=""><figcaption>LibTorch</figcaption></figure>
        <figure><img src="assets/images/engines/onnx.svg" alt=""><figcaption>ONNX Runtime</figcaption></figure>
        <figure><img src="assets/images/engines/tensorflow.svg" alt=""><figcaption>LiteRT</figcaption></figure>
        <figure><img src="assets/images/engines/pytorch.svg" alt=""><figcaption>ExecuTorch</figcaption></figure>
        <figure>{{ICON:code}}<figcaption>Custom</figcaption></figure>
      </div>
    </div>
    <div class="pitch-row fragment">
      <div class="pitch-sublabel">Devices</div>
      <div class="pitch-devices">
        <figure class="device desktop"><div class="device-screen"><img src="assets/images/platforms/apple.svg" alt="macOS"><img src="assets/images/platforms/windows.svg" alt="Windows"><img src="assets/images/platforms/linux.svg" alt="Linux"></div><figcaption>Desktop</figcaption></figure>
        <figure class="device phone"><div class="device-screen"><img src="assets/images/platforms/ios.svg" alt="iOS"><img src="assets/images/platforms/android.svg" alt="Android"></div><figcaption>Mobile</figcaption></figure>
        <figure class="device browser"><div class="device-screen"><img src="assets/images/platforms/webassembly.svg" alt="WebAssembly"></div><figcaption>Web</figcaption></figure>
        <figure class="device board"><div class="device-screen"><span>Bela</span></div><figcaption>Embedded<small>ARMv7 · ARM64</small></figcaption></figure>
      </div>
    </div>
    <div class="pitch-row fragment">
      <div class="pitch-sublabel">Hosts</div>
      <div class="pitch-hosts">
        <span>{{ICON:plug}}Audio plugins</span>
        <span>{{ICON:slides}}Standalone apps</span>
        <span>{{ICON:wave}}<code>anira~</code> in Max/MSP</span>
        <span>{{ICON:camera}}Real-time video</span>
      </div>
    </div>
  </div>
</div>

Note:
- anira is a C++ library: you describe the model and what the host promises, and anira takes care of threads, buffers and latency.
- [click] It runs LibTorch, ONNX Runtime, LiteRT, ExecuTorch, or your own engine.
- [click] On desktop, mobile, in the browser, and on boards like Bela.
- [click] In plugins, standalone apps, Max/MSP, and on real-time video.

---

<div class="tag">In production</div>

## anira in products.<br><span class="then">A selection.</span>

<div class="cards shipped">
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/drum-gate-2.jpg" alt=""></div><figcaption><span class="card-company">Sonnox · Plugin</span>Oxford Drum Gate 2</figcaption></figure>
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/subculture.jpg" alt=""></div><figcaption><span class="card-company">Baby Audio · Plugin</span>SubCulture</figcaption></figure>
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/lydia.jpg" alt=""></div><figcaption><span class="card-company">Neutone · Embedded</span>Project LYDIA</figcaption></figure>
</div>

Note:
- anira was a research project, but is not established in released products
- here is a selection:
- [click] Sonnox Oxford Drum Gate 2, which classifies drum hits by timbre.
- [click] Baby Audio SubCulture, a pitch-tracking bass enhancer.
- [click] And Project LYDIA by Neutone, a pedal on a Raspberry Pi 5.

---

<div class="tag">History</div>

## How anira evolved.<br><span class="then">From one thread per plugin to one C ABI.</span>

<div class="history">
  <div class="history-item current"><div class="history-year">2023</div><div class="history-name">ADC23</div><div class="history-text">one inference thread per plugin</div></div>
  <div class="history-item later"><div class="history-year">2024</div><div class="history-name">IS² paper</div><div class="history-text">anira, presented scientifically</div></div>
  <div class="history-item later"><div class="history-year">2024</div><div class="history-name">ADC24</div><div class="history-text">anira on stage</div></div>
  <div class="history-item later"><div class="history-year">07/2025</div><div class="history-name">v2</div><div class="history-text">any tensor, not only audio</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">Mobile</div><div class="history-text">iOS, Android and the web</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">v3 pre-release</div><div class="history-text">one C ABI; what this workshop runs</div></div>
  <div class="history-item later upcoming"><div class="history-year">end of 11/2026</div><div class="history-name">v3.0</div><div class="history-text">the release</div></div>
</div>

<div class="history-ask fragment">Have you thought about many instances?<br>Every plugin opens its own real-time thread.<span>the sonible team, after the talk</span></div>

Note:
- How did we get here? Not in one go.
- At ADC23 we showed a template with one inference thread per plugin: the design you built in part 1.
- It worked, for one plugin.
- [click] After the talk the sonible team asked us: have you thought about many instances?
- Every plugin opens its own real-time thread.

---

<div class="tag">anira v1</div>

## A library has to solve this, not a plugin.<br><span class="then">Eight instances, eight real-time threads, four cores.</span>

<div class="crowd">
  <div class="crowd-plugins">
    <div class="crowd-plugin" style="--i:0"><span>instance 1</span><i></i></div>
    <div class="crowd-plugin" style="--i:1"><span>instance 2</span><i></i></div>
    <div class="crowd-plugin" style="--i:2"><span>instance 3</span><i></i></div>
    <div class="crowd-plugin" style="--i:3"><span>instance 4</span><i></i></div>
    <div class="crowd-plugin over" style="--i:4"><span>instance 5</span><i></i></div>
    <div class="crowd-plugin over" style="--i:5"><span>instance 6</span><i></i></div>
    <div class="crowd-plugin over" style="--i:6"><span>instance 7</span><i></i></div>
    <div class="crowd-plugin over" style="--i:7"><span>instance 8</span><i></i></div>
  </div>
  <div class="crowd-cores">
    <span>4 cores</span>
    <b style="--i:0"></b><b style="--i:1"></b><b style="--i:2"></b><b style="--i:3"></b>
  </div>
  <div class="crowd-count"><strong>8</strong> real-time threads on <strong>4</strong> cores</div>
</div>

<div class="solutions fragment">
  <div class="solution"><span>In a plugin</span>each instance starts its own thread and cannot see the others</div>
  <div class="solution library"><span>In a library</span>one thread pool per process, shared by every instance</div>
</div>

Note:
- One plugin, one thread: eight instances on four cores make eight real-time threads.
- Four of them have to wait, and their blocks come too late.
- [click] A plugin cannot fix this, a library can: one thread pool for the whole process.
- That was anira v1.

---

<div class="tag">History</div>

## How anira evolved.<br><span class="then">From one thread per plugin to one C ABI.</span>

<div class="history">
  <div class="history-item"><div class="history-year">2023</div><div class="history-name">ADC23</div><div class="history-text">one inference thread per plugin</div></div>
  <div class="history-item current"><div class="history-year">2024</div><div class="history-name">IS² paper</div><div class="history-text">anira, presented scientifically</div></div>
  <div class="history-item current"><div class="history-year">2024</div><div class="history-name">ADC24</div><div class="history-text">anira on stage</div></div>
  <div class="history-item later"><div class="history-year">07/2025</div><div class="history-name">v2</div><div class="history-text">any tensor, not only audio</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">Mobile</div><div class="history-text">iOS, Android and the web</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">v3 pre-release</div><div class="history-text">one C ABI; what this workshop runs</div></div>
  <div class="history-item later upcoming"><div class="history-year">end of 11/2026</div><div class="history-name">v3.0</div><div class="history-text">the release</div></div>
</div>

Note:
- That is what we presented at ADC24, and published as a paper at IS², where it won the Industry Award.

---

<div class="tag">History</div>

## How anira evolved.<br><span class="then">From one thread per plugin to one C ABI.</span>

<div class="history">
  <div class="history-item"><div class="history-year">2023</div><div class="history-name">ADC23</div><div class="history-text">one inference thread per plugin</div></div>
  <div class="history-item"><div class="history-year">2024</div><div class="history-name">IS² paper</div><div class="history-text">anira, presented scientifically</div></div>
  <div class="history-item"><div class="history-year">2024</div><div class="history-name">ADC24</div><div class="history-text">anira on stage</div></div>
  <div class="history-item current"><div class="history-year">07/2025</div><div class="history-name">v2</div><div class="history-text">any tensor, not only audio</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">Mobile</div><div class="history-text">iOS, Android and the web</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">v3 pre-release</div><div class="history-text">one C ABI; what this workshop runs</div></div>
  <div class="history-item later upcoming"><div class="history-year">end of 11/2026</div><div class="history-name">v3.0</div><div class="history-text">the release</div></div>
</div>

Note:
- In July 2025, with v2, we expanded anira from an audio-to-audio solution to any kind of data.
- Models started taking control data next to audio, like pitch and loudness, one value per frame instead of one per sample.
- So in v2, every input and output became a tensor with its own shape and its own rate.

---

<div class="tag">History</div>

## How anira evolved.<br><span class="then">From one thread per plugin to one C ABI.</span>

<div class="history">
  <div class="history-item"><div class="history-year">2023</div><div class="history-name">ADC23</div><div class="history-text">one inference thread per plugin</div></div>
  <div class="history-item"><div class="history-year">2024</div><div class="history-name">IS² paper</div><div class="history-text">anira, presented scientifically</div></div>
  <div class="history-item"><div class="history-year">2024</div><div class="history-name">ADC24</div><div class="history-text">anira on stage</div></div>
  <div class="history-item"><div class="history-year">07/2025</div><div class="history-name">v2</div><div class="history-text">any tensor, not only audio</div></div>
  <div class="history-item current"><div class="history-year">2026</div><div class="history-name">Mobile</div><div class="history-text">iOS, Android and the web</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">v3 pre-release</div><div class="history-text">one C ABI; what this workshop runs</div></div>
  <div class="history-item later upcoming"><div class="history-year">end of 11/2026</div><div class="history-name">v3.0</div><div class="history-text">the release</div></div>
</div>

<div class="history-note">funded by the BMFTR since 2026</div>

Note:
- In 2026 we added webassembly support for the browser
- And we got public funding from the BMFTR to develop anira further.
- from an audio only solution to video, image and text
- With it came iOS, Android
- And a new design: v3.

---

<div class="tag">anira v3</div>

## Where v2 reached its limits.<br><span class="then">What v3 changes.</span>

<div class="limits">
  <div class="limit">
    {{ICON:model}}
    <div class="limit-v2"><span>v2</span>audio buffers, in float</div>
    <div class="limit-v3 fragment"><span>v3</span><div class="chips"><b>audio</b><b>image</b><b>text</b></div>typed tensors, images as frames</div>
  </div>
  <div class="limit">
    {{ICON:code}}
    <div class="limit-v2"><span>v2</span>TypeScript wraps the C++ API</div>
    <div class="limit-v3 fragment"><span>v3</span><div class="chips"><b class="core">C ABI</b><i>→</i><b>C++</b><b>TypeScript</b></div>one versioned binary contract</div>
  </div>
  <div class="limit">
    {{ICON:clock}}
    <div class="limit-v2"><span>v2</span>built around the audio stream</div>
    <div class="limit-v3 fragment"><span>v3</span><div class="chips"><b>Hard · stream</b><b>Async · job</b></div>two contracts, both fast</div>
  </div>
  <div class="limit">
    {{ICON:chip}}
    <div class="limit-v2"><span>v2</span>data passes through CPU memory</div>
    <div class="limit-v3 fragment"><span>v3</span><div class="chips"><b>CPU</b><b>GPU</b><b>NPU</b></div>one tensor type on every device</div>
  </div>
</div>

Note:
- v2 was built for audio plugins on the desktop, and that is where it reached its limits.
- [click] v3 takes any data: audio, images and text.
- [click] One C ABI at the core, and C++ and TypeScript are generated from it.
- [click] Two contracts: Hard for a stream, Async for jobs.
- [click] And one tensor type for CPU, GPU and NPU.

---

<div class="tag">History</div>

## How anira evolved.<br><span class="then">From one thread per plugin to one C ABI.</span>

<div class="history">
  <div class="history-item"><div class="history-year">2023</div><div class="history-name">ADC23</div><div class="history-text">one inference thread per plugin</div></div>
  <div class="history-item"><div class="history-year">2024</div><div class="history-name">IS² paper</div><div class="history-text">anira, presented scientifically</div></div>
  <div class="history-item"><div class="history-year">2024</div><div class="history-name">ADC24</div><div class="history-text">anira on stage</div></div>
  <div class="history-item"><div class="history-year">07/2025</div><div class="history-name">v2</div><div class="history-text">any tensor, not only audio</div></div>
  <div class="history-item"><div class="history-year">2026</div><div class="history-name">Mobile</div><div class="history-text">iOS, Android and the web</div></div>
  <div class="history-item current"><div class="history-year">2026</div><div class="history-name">v3 pre-release</div><div class="history-text">one C ABI; what this workshop runs</div></div>
  <div class="history-item current upcoming"><div class="history-year">end of 11/2026</div><div class="history-name">v3.0</div><div class="history-text">the release</div></div>
</div>

<div class="history-note">Apache 2.0 · tanh lab, TU Berlin</div>

Note:
- Today you run a v3 pre-release, and v3.0 ships at the end of November.
- After the release we want to go back to research: for real-time audio and video, is the CPU, the GPU or the NPU faster?
- GPU and NPU inference is a whole talk of its own, so we cannot cover it here.
- If you have already worked on this, come and talk to us, we would love to hear about your experience.
- Now Fares shows you how anira works inside.

