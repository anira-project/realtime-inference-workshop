<h1>anira</h1>

Replacing the hand-built parts

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->
<!-- part: anira -->

Note:
    - Part 2. Everything in part 1 was built so that this part can name it.
    - Talk first: what anira is, then the concepts one by one, then the architecture. Hands-on after that.

---

<div class="tag">Part 1</div>

## Six things real-time inference needs.<br><span class="then">We solved each one, the simple way.</span>

<div class="needs">
  <div class="card need">{{ICON:model}}<div class="card-title">Inference</div><div class="card-text">steps 1, 2, 4 · one engine, one block size</div></div>
  <div class="card need">{{ICON:gauge}}<div class="card-title">Benchmarking</div><div class="card-text">step 3 · one benchmark, on our machine</div></div>
  <div class="card need">{{ICON:shield}}<div class="card-title">Real-time safety</div><div class="card-text">step 5 · RTSan, run once</div></div>
  <div class="card need">{{ICON:threads}}<div class="card-title">Threading</div><div class="card-text">steps 6, 8 · a worker per channel</div></div>
  <div class="card need">{{ICON:clock}}<div class="card-title">Minimal latency</div><div class="card-text">step 7 · a prefill, counted by hand</div></div>
  <div class="card need">{{ICON:flag}}<div class="card-title">Real-time guarantees</div><div class="card-text">steps 3, 7 · hoping the budget holds</div></div>
  <div class="needs-anira fragment"><span>anira</span> covers all six, for one plugin instance or many.</div>
</div>

Note:
    - What we did in part 1, as six needs. Every one works, and every one is the simple version: one engine, one machine, one instance.
    - Click once: anira covers all six, from years of plugins in production. Not how yet; the next two chapters open them one by one.
    - The dry path is the one thing that stays in your code: anira reports the number, the mix is the plugin's.

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
    - What part 1 built by hand is the inside of anira. You only say what the model is and what the host promises; the rest is the library's.
    - Audio plugins first, and real-time video since the async contract: step 11 runs hand tracking on camera frames.
    - Since v3 the library is one versioned C ABI. The C++ API and the TypeScript package on npm are generated from it, so every language sees the same library.
    - Click: every engine, or your own. Click: desktop, mobile, the browser, and embedded boards like Bela, on ARMv7 and ARM64. Click: in a plugin or a standalone app, as the anira~ external in Max/MSP, or on camera frames. Solve it once.

---

<div class="tag">In production</div>

## anira in products.<br><span class="then">A selection.</span>

<div class="cards shipped">
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/drum-gate-2.jpg" alt=""></div><figcaption><span class="card-company">Sonnox · Plugin</span>Oxford Drum Gate 2</figcaption></figure>
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/subculture.jpg" alt=""></div><figcaption><span class="card-company">Baby Audio · Plugin</span>SubCulture</figcaption></figure>
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/lydia.jpg" alt=""></div><figcaption><span class="card-company">Neutone · Embedded</span>Project LYDIA</figcaption></figure>
</div>

Note:
    - Sonnox Oxford Drum Gate 2 (February 2026): the gate classifies drum hits by timbre, not level. Its models run through anira.
    - Baby Audio SubCulture (July 2026): pitch-tracking bass enhancer; its models run through anira.
    - Project LYDIA by Neutone: Neutone's Morpho on a Raspberry Pi 5, the embedded case from the slide before. A hardware project, not a product on sale yet.
    - Three of more: plugins on every desktop host and a pedal on ARM, the same library in all of them. More products use anira than we can show here.
    - Images: the Sonnox hero shot and the Baby Audio screenshot from their product pages, the pedal from Neutone.

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
    - ADC23 (London): the nn-inference-template, one inference thread per plugin. It worked, for one plugin.
    - After the talk, the team from sonible, an Austrian plugin company, asked what happens with many instances. Every instance opening its own real-time thread was the part we had not thought through.
    - The point: a design like this is not found in one attempt. It came from iterating, and from talking to people who ship plugins; don't hold an idea back for fear someone takes it. The next slide shows the question in detail.

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
    - The question from sonible, drawn. Step 8 ended here: one engine and one worker per channel, per instance. The host does not know, and keeps adding instances.
    - Real-time threads that outnumber cores take turns. A worker that is not scheduled misses its block, and the miss lands in someone else's plugin.
    - Click: a plugin cannot fix this from inside, because one instance does not see the others. The fix is a static design in a shared library: one pool per process that every instance uses, and ideally every product that loads the same library. That was the idea behind anira v1. The details follow in the next chapter.

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
    - IS² 2024: the static thread pool presented scientifically, as a paper; it won the Industry Award.

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
    - ADC24 in Bristol: anira on stage, for the developers who asked the question in the first place.

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
    - Models started to take control data: pitch and loudness estimated from the input, driving the model alongside or instead of the audio.
    - Those values come once per analysis frame, far below the audio rate. A buffer that assumes one sample in, one sample out cannot carry them.
    - v2 made every input and output a tensor with its own shape and rate, so the block-to-model adaptation is done per tensor. Getting the rates right across host block sizes was the work.

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
    - v2 (07/2025): arbitrary tensors, non-audio inputs and outputs, different rates in and out.

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
    - 2026, mobile: iOS and Android, the browser through WASM, LiteRT and ExecuTorch, generators and analysers, a closed-form latency (v2.1 to v2.3).
    - Each of these added a special case to a design built for audio plugins on the desktop.

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
    <div class="qr-label">Paper</div>
    <div class="qr-url">link follows</div>
  </div>
</div>

Note:
    - TODO: dummy slide. Replace the QR target with the paper's URL and the title with the paper's title once it is out.
    - For anyone interested in how the thread pool and the real-time path work in a browser: the details are in the paper.

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
    - From the v3 architecture document in the anira repo (docs/anira-v3-architecture.md).
    - The TypeScript package wrapped C++ directly. A library that other languages and hosts load needs one stable, versioned C ABI; the C++ API and the TypeScript package are generated on top of it.
    - Data beyond audio: v2 opened the shapes, but the library still thought in float buffers. v3 types every tensor (DLPack dtypes: float, int, bool and more, so token IDs for text fit), and image data enters as a Frame that a FrameToTensor stage turns into a tensor. That meant rebuilding the core around one typed tensor.
    - Some work is not a stream: a camera frame, an analysis, a generation. Async jobs with a deadline next to the Hard real-time contract.
    - One passive tensor type that carries memory, ownership and readiness across CPU, GPU and NPU. A planner at prepare compiles what was declared into a fixed pipeline.
    - The real-time audio path is unchanged: the nonblocking entries forward to the v2 code paths and never wait.

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
    - the engines anira wraps, each with the accelerators of the prebuilt packages in anira-project/backends: CPU by default, GPU in the -gpu and -cuda variants. ONNX Runtime with CUDA, CoreML, DirectML and WebGPU; LibTorch with CUDA and MPS; LiteRT with Metal, WebGPU and OpenCL/GL; TFLite with Metal, CoreML and OpenCL delegates; ExecuTorch with CoreML, MPS, MLX and Vulkan. Which one is available depends on the platform (see the matrix in the backends README).
    - Which combination reaches real-time for audio is an open question: on the CPU it is measured, on the GPU barely. That is the outlook.

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
    - Everything in this part runs on a v3 pre-release (after v3.0.0-alpha.1); v3.0 ships at the end of November. Check the release before you build on it.
    - Since 2026 funded by the BMFTR through the Prototype Fund; Lina joined the team.

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
    - Every engine runs on several providers, every provider on several kinds of hardware. Which path is fastest for a given model is not known in advance; kernel quality differs per engine, model class and driver.
    - Click: a few of the candidate paths. Each one is a question, not an answer.
    - Click: for audio, the CPU paths are measured and meet real-time. The GPU paths are barely studied for real-time audio, where the transfer and the launch cost count against a block of a few milliseconds.
    - After the v3.0 release at the end of November we benchmark these combinations systematically; anira's benchmark tools and the plan candidates of v3 are built for exactly that sweep.
    - Click: anyone who has numbers for one of these paths, or a model that behaves differently, please talk to us. Same lesson as with the thread pool.

---

<div class="tag">How you use it</div>

## Three configs say what, how, and with what.<br><span class="then">The handler runs it.</span>

<div class="cards">
  <div class="card">{{ICON:model}}<div class="card-title">ModelConfig</div><div class="card-text">what: the files, every tensor, its role</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:sliders}}<div class="card-title">Contract</div><div class="card-text">how: <em>Hard</em> for a stream, <em>Async</em> for jobs</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:chip}}<div class="card-title">ContextConfig</div><div class="card-text">with what: threads, engines, log</div></div>
  <div class="card-arrow">→</div>
  <div class="card accent">{{ICON:shield}}<div class="card-title">Handler</div><div class="card-text"><code>prepare</code> · <code>process</code> · <code>get_latency</code></div></div>
</div>

<div class="cards-note">Two JSON files and three calls. The rest is the library's. Today: a v3 pre-release, pinned by CMake. v3.0 ships at the end of November.</div>

Note:
    - ModelConfig: which files, which engines may run them, every tensor with a name and a role (streamed, static, state).
    - Contract: what the host promises and what it gets. Hard: a block size range, a time budget, what happens on a miss; the latency follows from it. Async: a job with a deadline, for things like a camera frame. Everything in the hands-on is Hard, except the hand tracking in step 11.
    - Version: the workshop pins a v3 pre-release commit (after v3.0.0-alpha.1); CMake fetches it with ONNX Runtime built in. The proper v3.0 release follows at the end of November; details may still move until then.
    - ContextConfig: the process-wide part, threads and enabled engines. One per process, like the pool.
    - Handler: the three calls from step 8, prepareToPlay, processBlock and setLatencySamples, one each.

---

<div class="tag">Take away</div>

## Three kinds of thread.<br><span class="then">Each does one thing.</span>

<div class="statement-points">
  <div>The main thread plans: configs, <code>prepare</code>, the latency</div>
  <div>The audio thread moves samples: <code>process</code>, nothing else</div>
  <div>The pool runs the models</div>
</div>

Note:
    - That is the whole library in one sentence. The next two chapters go through the concepts behind it, each one starting from the step in part 1 that built it by hand.
