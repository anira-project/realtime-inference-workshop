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
  <div class="card need">{{ICON:model}}<div class="card-title">Inference</div><div class="card-text">steps 1, 2, 7 · one engine, one block size</div></div>
  <div class="card need">{{ICON:gauge}}<div class="card-title">Benchmarking</div><div class="card-text">step 3 · one benchmark, on our machine</div></div>
  <div class="card need">{{ICON:shield}}<div class="card-title">Real-time safety</div><div class="card-text">step 4 · RTSan, run once</div></div>
  <div class="card need">{{ICON:threads}}<div class="card-title">Threading</div><div class="card-text">steps 5, 8 · a worker per channel</div></div>
  <div class="card need">{{ICON:clock}}<div class="card-title">Minimal latency</div><div class="card-text">step 6 · a prefill, counted by hand</div></div>
  <div class="card need">{{ICON:flag}}<div class="card-title">Real-time guarantees</div><div class="card-text">steps 3, 6 · hoping the budget holds</div></div>
  <div class="needs-anira fragment"><span>anira</span> covers all six, for one plugin instance or many.</div>
</div>

Note:
- Here is part 1 in one picture: six things real-time inference needs.
- Inference, benchmarking, real-time safety, threading, minimal latency, and real-time guarantees.
- We solved each one the simple way: one engine, one machine, one plugin instance.
- That is fine for one instance, not for a session full of them.
- [click] anira covers all six, for one instance or for many.
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
- anira is a C++ library.
- You describe the model in a model file and what the host promises in a contract file.
- An optional third file, the context file, comes later in Fares's part.
- Threads, buffers, latency and safety are then the library's job.
- In v3, the version you run today, the core is one C ABI, and the C++ API and the TypeScript package are generated from it.
- [click] It runs LibTorch, ONNX Runtime, LiteRT, ExecuTorch, or your own engine.
- [click] It runs on desktop, mobile, in the browser, and on embedded boards like Bela.
- [click] And it runs in plugins, standalone apps, as anira~ in Max/MSP, and on real-time video.

---

<div class="tag">In production</div>

## anira in products.<br><span class="then">A selection.</span>

<div class="cards shipped">
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/drum-gate-2.jpg" alt=""></div><figcaption><span class="card-company">Sonnox · Plugin</span>Oxford Drum Gate 2</figcaption></figure>
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/subculture.jpg" alt=""></div><figcaption><span class="card-company">Baby Audio · Plugin</span>SubCulture</figcaption></figure>
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/lydia.jpg" alt=""></div><figcaption><span class="card-company">Neutone · Embedded</span>Project LYDIA</figcaption></figure>
</div>

Note:
- anira in products, a selection.
- [click] Sonnox Oxford Drum Gate 2, which classifies drum hits by timbre.
- [click] Baby Audio SubCulture, a pitch-tracking bass enhancer.
- [click] And Project LYDIA by Neutone, a pedal on a Raspberry Pi 5.
- These are three of several, on desktop hosts and on ARM boards.

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

<div class="history-note lesson fragment">Good solutions come from iteration and exchange, not from a single attempt.</div>

Note:
- How did we get here? Not in one go.
- At ADC23 we showed a template with one inference thread per plugin: the design you built in part 1.
- It worked, for one plugin.
- [click] After the talk the sonible team asked us: have you thought about many instances?
- Every plugin opens its own real-time thread.
- [click] That is the lesson of this whole history: good solutions come from iteration and exchange, not from a single attempt.

---

<div class="tag">anira v1</div>

## A library has to solve this, not a plugin.<br><span class="then">Ten instances, twenty real-time threads, eight cores.</span>

<div class="crowd">
  <div class="crowd-plugins">
    <div class="crowd-plugin" style="--i:0"><span>instance 1</span><i></i><i></i></div>
    <div class="crowd-plugin" style="--i:1"><span>instance 2</span><i></i><i></i></div>
    <div class="crowd-plugin" style="--i:2"><span>instance 3</span><i></i><i></i></div>
    <div class="crowd-plugin" style="--i:3"><span>instance 4</span><i></i><i></i></div>
    <div class="crowd-plugin over" style="--i:4"><span>instance 5</span><i></i><i></i></div>
    <div class="crowd-plugin over" style="--i:5"><span>instance 6</span><i></i><i></i></div>
    <div class="crowd-plugin over" style="--i:6"><span>instance 7</span><i></i><i></i></div>
    <div class="crowd-plugin over" style="--i:7"><span>instance 8</span><i></i><i></i></div>
    <div class="crowd-plugin over" style="--i:8"><span>instance 9</span><i></i><i></i></div>
    <div class="crowd-plugin over" style="--i:9"><span>instance 10</span><i></i><i></i></div>
  </div>
  <div class="crowd-cores">
    <span>8 cores</span>
    <b style="--i:0"></b><b style="--i:1"></b><b style="--i:2"></b><b style="--i:3"></b>
    <b style="--i:4"></b><b style="--i:5"></b><b style="--i:6"></b><b style="--i:7"></b>
  </div>
  <div class="crowd-count"><strong>20</strong> real-time threads on <strong>8</strong> cores</div>
</div>

<div class="solutions fragment">
  <div class="solution"><span>In a plugin</span>each instance starts and sizes its own threads; it cannot see the other instances</div>
  <div class="solution library"><span>In a library, static</span>one thread pool per process, sized to the machine; every instance queues into it, ideally across products</div>
</div>

Note:
- This is their question, drawn out: our step 8 plugin, ten times.
- Ten instances with two workers each make twenty real-time threads on eight cores.
- A thread that is not scheduled in time misses its block, and the miss lands in someone else's plugin.
- [click] A plugin cannot fix this, because one instance cannot see the others.
- A library can: one thread pool shared by the whole process, sized to the machine, and every instance queues into it.
- That was anira v1.

---

<div class="tag">History</div>

## How anira evolved.<br><span class="then">From one thread per plugin to one C ABI.</span>

<div class="history">
  <div class="history-item"><div class="history-year">2023</div><div class="history-name">ADC23</div><div class="history-text">one inference thread per plugin</div></div>
  <div class="history-item current"><div class="history-year">2024</div><div class="history-name">IS² paper</div><div class="history-text">anira, presented scientifically</div></div>
  <div class="history-item later"><div class="history-year">2024</div><div class="history-name">ADC24</div><div class="history-text">anira on stage</div></div>
  <div class="history-item later"><div class="history-year">07/2025</div><div class="history-name">v2</div><div class="history-text">any tensor, not only audio</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">Mobile</div><div class="history-text">iOS, Android and the web</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">v3 pre-release</div><div class="history-text">one C ABI; what this workshop runs</div></div>
  <div class="history-item later upcoming"><div class="history-year">end of 11/2026</div><div class="history-name">v3.0</div><div class="history-text">the release</div></div>
</div>

Note:
- In 2024 we published that design as a paper at IS², the IEEE International Symposium on the Internet of Sounds, and it won the Industry Award.

---

<div class="tag">History</div>

## How anira evolved.<br><span class="then">From one thread per plugin to one C ABI.</span>

<div class="history">
  <div class="history-item"><div class="history-year">2023</div><div class="history-name">ADC23</div><div class="history-text">one inference thread per plugin</div></div>
  <div class="history-item"><div class="history-year">2024</div><div class="history-name">IS² paper</div><div class="history-text">anira, presented scientifically</div></div>
  <div class="history-item current"><div class="history-year">2024</div><div class="history-name">ADC24</div><div class="history-text">anira on stage</div></div>
  <div class="history-item later"><div class="history-year">07/2025</div><div class="history-name">v2</div><div class="history-text">any tensor, not only audio</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">Mobile</div><div class="history-text">iOS, Android and the web</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">v3 pre-release</div><div class="history-text">one C ABI; what this workshop runs</div></div>
  <div class="history-item later upcoming"><div class="history-year">end of 11/2026</div><div class="history-name">v3.0</div><div class="history-text">the release</div></div>
</div>

Note:
- The same year we brought anira to ADC24 in Bristol, back to the people who had asked the question.

---

<div class="tag">anira v2</div>

## Control data next to audio.<br><span class="then">Inputs at different rates, in one model.</span>

<div class="rates">
  <div class="rates-label">audio</div>
  <div class="rates-track audio"></div>
  <div class="rates-rate">every sample</div>
  <div class="rates-label">pitch · loudness</div>
  <div class="rates-track control"></div>
  <div class="rates-rate">one value per frame</div>
  <div class="rates-model">{{ICON:model}}model</div>
  <div class="rates-out">→ audio</div>
</div>

<div class="cards-note">v2: every input and output is a tensor with its own shape and rate, not only an audio buffer.</div>

Note:
- Then the models changed.
- They started taking control data such as pitch and loudness, one value per frame instead of one per sample.
- v1 was built around one audio buffer in and one out, at the same rate.
- In v2, every input and output became a tensor with its own shape and its own rate.

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
- That was v2, in July 2025.

---

<div class="tag">anira on the web</div>

## A multithreaded library in the browser.<br><span class="then">How we ported anira to WebAssembly.</span>

<div class="paper">
  <div class="paper-text">
    <div class="paper-kind">Paper · published next week</div>
    <div class="paper-title">Porting a multithreaded real-time inference library to the web</div>
    <div class="paper-points">Threads, shared memory and the audio worklet in the browser, and what that means for the pool.</div>
  </div>
  <div class="qr">
    {{QR:HTTPS://GITHUB.COM/ANIRA-PROJECT/ANIRA}}
    <div class="qr-label">anira on GitHub</div>
    <div class="qr-url">the paper is linked there once it is out</div>
  </div>
</div>

Note:
- In 2026 we took anira off the desktop, and one target was the browser.
- The hard part there is threads: shared memory, and an audio worklet that must never block.
- The paper on how we solved that comes out next week; the link will be on GitHub.

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

Note:
- Next to the browser, 2026 also brought iOS and Android.
- Every one of these added a special case to a design built for desktop plugins.

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
- Four places, then, where v2 reached its limits.
- [click] Data: from float audio buffers to typed tensors, with images and text too.
- [click] Bindings: from TypeScript wrapping C++ to one versioned C ABI.
- [click] Timing: next to the hard audio stream, async jobs with a deadline.
- [click] Memory: one tensor type for CPU, GPU and NPU.

---

<div class="tag">anira v3</div>

## Engines and accelerators.<br><span class="then">CPU by default, GPU in the -gpu and -cuda packages.</span>

<div class="engine-matrix">
  <div class="mx-corner"></div>
  <div class="mx-col">CPU</div>
  <div class="mx-col">CUDA</div>
  <div class="mx-col">CoreML</div>
  <div class="mx-col">MPS</div>
  <div class="mx-col">MLX</div>
  <div class="mx-col">Metal</div>
  <div class="mx-col">DirectML</div>
  <div class="mx-col">Vulkan</div>
  <div class="mx-col">WebGPU</div>
  <div class="mx-col">OpenCL</div>
  <div class="mx-engine">ONNX Runtime</div>
  <div class="mx-yes"></div>
  <div class="mx-yes"></div>
  <div class="mx-yes"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-yes"></div>
  <div class="mx-no"></div>
  <div class="mx-yes"></div>
  <div class="mx-no"></div>
  <div class="mx-engine">LibTorch</div>
  <div class="mx-yes"></div>
  <div class="mx-yes"></div>
  <div class="mx-no"></div>
  <div class="mx-yes"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-engine">LiteRT</div>
  <div class="mx-yes"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-yes"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-yes"></div>
  <div class="mx-yes"></div>
  <div class="mx-engine">TFLite</div>
  <div class="mx-yes"></div>
  <div class="mx-no"></div>
  <div class="mx-yes"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-yes"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-yes"></div>
  <div class="mx-engine">ExecuTorch</div>
  <div class="mx-yes"></div>
  <div class="mx-no"></div>
  <div class="mx-yes"></div>
  <div class="mx-yes"></div>
  <div class="mx-yes"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
  <div class="mx-yes"></div>
  <div class="mx-no"></div>
  <div class="mx-no"></div>
</div>

<div class="cards-note">Prebuilt in anira-project/backends; which one is available depends on the platform. Any other engine can be registered by name.</div>

Note:
- This is what that opens up: five engines, each with its own set of accelerators.
- TFLite is the same runtime as LiteRT through its older API, which is why the earlier slide showed four.
- No engine covers every accelerator, so choosing an engine also means choosing which accelerators you can use.
- CPU is the default; the GPU paths come in the -gpu and -cuda packages.
- They are prebuilt in the backends repository, as far as each platform allows, and any other engine can be registered by name.

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

<div class="history-note">Apache 2.0 · tanh lab, TU Berlin · funded by BMFTR since 2026</div>

Note:
- Today you run a v3 pre-release, and v3.0 ships at the end of November.
- anira is Apache 2.0, from tanh lab at TU Berlin, and since this year it is funded by the German Federal Ministry of Research, Technology and Space, the BMFTR.

---

<div class="tag">Outlook</div>

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
- That leaves an open question.
- Hardware, execution provider, engine: which path is fastest for a given model?
- [click] Here are a few of the candidate paths; each one is a question, not an answer.
- [click] For audio, the CPU paths are measured and they meet real time.
- On the GPU it has barely been studied.
- After v3.0 we will benchmark these combinations systematically.
- [click] If you have numbers for any of these, come and talk to us.
- The thread pool came out of one conversation after a talk, and we would like the next idea to come the same way.
- Now Fares shows you how anira works inside.
