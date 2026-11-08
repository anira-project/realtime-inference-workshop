<h1>anira</h1>

Replacing the hand-built parts

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->
<!-- part: anira -->

Note:
- Welcome back.
- In part 1 you built inference for one case.
- One model, one host block size, one fixed model input size.
- Change any of the three, and you redo many of those steps.
- That's why we built anira, to simplify that process.
- In part 2, I first show what anira is and where it comes from.
- Then Fares shows you how it works inside.
- After that, you use anira in the exercises, starting with step 9.

---

<div class="tag">Part 1</div>

## Six things real-time inference needs.<br><span class="then">We solved each one, the simple way.</span>

<div class="needs">
  <div class="card need">{{ICON:model}}<div class="card-title">Inference</div><div class="card-text">steps 1, 2, 3 · one engine, one block size</div></div>
  <div class="card need">{{ICON:gauge}}<div class="card-title">Benchmarking</div><div class="card-text">step 4 · one benchmark, on our machine</div></div>
  <div class="card need">{{ICON:shield}}<div class="card-title">Real-time safety</div><div class="card-text">step 5 · RTSan, run once</div></div>
  <div class="card need">{{ICON:threads}}<div class="card-title">Threading</div><div class="card-text">steps 6, 8 · a worker per plugin instance</div></div>
  <div class="card need">{{ICON:clock}}<div class="card-title">Minimal latency</div><div class="card-text">step 7 · a prefill, counted by hand</div></div>
  <div class="card need">{{ICON:flag}}<div class="card-title">Real-time guarantees</div><div class="card-text">steps 4, 7 · hoping the budget holds</div></div>
  <div class="needs-anira fragment"><span>anira</span> covers all six, for one plugin instance or many.</div>
</div>

Note:
- In part 1 we built six things by hand.
- We ran the inference.
- We benchmarked it on our machine.
- We checked the engines for real-time safety.
- We gave each plugin instance one worker thread.
- We calculated the minimal latency.
- And we made sure the real-time thread is never blocked.
- [pause, then click] anira covers all six, for one plugin instance or many.
- So: what is anira?

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
- You describe the model, and what the host promises.
- anira takes care of threads, buffers and latency.
- Underneath is one C ABI [say: C, A-B-I], with C++ and TypeScript on top.
- [click] anira has the most common inference engines built in, and you can also bring your own.
- [click] It runs on desktop, mobile, in the browser, and on embedded platforms like Bela or a Raspberry Pi.
- [click] We also provide examples of how to run anira in an audio plugin or a standalone app.
- There is a Max/MSP external, and even a demo of real-time video inference.

---

<div class="tag">In production</div>

## anira in products.<br><span class="then">A selection.</span>

<div class="cards shipped">
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/drum-gate-2.jpg" alt=""></div><figcaption><span class="card-company">Sonnox · Plugin</span>Oxford Drum Gate 2</figcaption></figure>
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/subculture.jpg" alt=""></div><figcaption><span class="card-company">Baby Audio · Plugin</span>SubCulture</figcaption></figure>
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/lydia.jpg" alt=""></div><figcaption><span class="card-company">Neutone · Embedded</span>Project LYDIA</figcaption></figure>
</div>

Note:
- anira started as a research project.
- Nowadays it already ships in released products.
- [click] Sonnox Oxford Drum Gate 2 uses anira to classify drum hits.
- [click] In Baby Audio SubCulture, it helps tracking the pitch.
- [click] And Neutone uses it on an embedded device for timbre transfer, in Project LYDIA.
- So how did we get here? Well, not in one go.

---

<div class="tag">History</div>

## How anira evolved.<br><span class="then">From one thread per plugin to one C ABI.</span>

<div class="history">
  <div class="history-item current"><div class="history-year">2023</div><div class="history-name">ADC23</div><div class="history-text">one inference thread per plugin</div></div>
  <div class="history-item later"><div class="history-year">2024</div><div class="history-name">IS² paper</div><div class="history-text">anira, presented scientifically</div></div>
  <div class="history-item later"><div class="history-year">2024</div><div class="history-name">ADC24</div><div class="history-text">anira v1 on stage</div></div>
  <div class="history-item later"><div class="history-year">07/2025</div><div class="history-name">v2</div><div class="history-text">any tensor, not only audio</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">Mobile · Web</div><div class="history-text">iOS, Android and the web</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">v3 pre-release</div><div class="history-text">one C ABI; what this workshop runs</div></div>
  <div class="history-item later upcoming"><div class="history-year">end of 11/2026</div><div class="history-name">v3.0</div><div class="history-text">the release</div></div>
</div>

<div class="history-ask fragment">Have you thought about many instances?<br>Every plugin opens its own real-time thread.<span>the sonible team, after the talk</span></div>

Note:
- The timeline runs from 2023 to the end of this month!
- First stop, ADC23: we showed a plugin template.
- It is the design you built in part 1: a worker, a ring buffer each way, a prefill.
- It worked, for one plugin.
- [click] After the talk the sonible team asked: have you thought about many instances?
- Each instance spawns its own real-time thread!
- A fair question, so let's look at what it means.

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
- Let's imagine we suddenly have eight instances of our plugin
- But only four logical cores on our hardware.
- Then we run into a problem called oversubscription: more real-time threads than cores.
- The OS has to step in and give each thread its time to run, which drags down overall performance.
- [click] To address this, we need a solution that works across all instances.
- Ideally across multiple products.
- Solution: A static thread pool, which all products can use to run their inference.
- Therefore we decided to develop a library to do that.
- That was anira v1.

---

<div class="tag">History</div>

## How anira evolved.<br><span class="then">From one thread per plugin to one C ABI.</span>

<div class="history">
  <div class="history-item"><div class="history-year">2023</div><div class="history-name">ADC23</div><div class="history-text">one inference thread per plugin</div></div>
  <div class="history-item current"><div class="history-year">2024</div><div class="history-name">IS² paper</div><div class="history-text">anira, presented scientifically</div></div>
  <div class="history-item current"><div class="history-year">2024</div><div class="history-name">ADC24</div><div class="history-text">anira v1 on stage</div></div>
  <div class="history-item later"><div class="history-year">07/2025</div><div class="history-name">v2</div><div class="history-text">any tensor, not only audio</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">Mobile · Web</div><div class="history-text">iOS, Android and the web</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">v3 pre-release</div><div class="history-text">one C ABI; what this workshop runs</div></div>
  <div class="history-item later upcoming"><div class="history-year">end of 11/2026</div><div class="history-name">v3.0</div><div class="history-text">the release</div></div>
</div>

Note:
- This library design then was presented and published as paper at the Internet of Sounds conference in 2024.
- And we talked about it at ADC24

---

<div class="tag">History</div>

## How anira evolved.<br><span class="then">From one thread per plugin to one C ABI.</span>

<div class="history">
  <div class="history-item"><div class="history-year">2023</div><div class="history-name">ADC23</div><div class="history-text">one inference thread per plugin</div></div>
  <div class="history-item"><div class="history-year">2024</div><div class="history-name">IS² paper</div><div class="history-text">anira, presented scientifically</div></div>
  <div class="history-item"><div class="history-year">2024</div><div class="history-name">ADC24</div><div class="history-text">anira v1 on stage</div></div>
  <div class="history-item current"><div class="history-year">07/2025</div><div class="history-name">v2</div><div class="history-text">any tensor, not only audio</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">Mobile · Web</div><div class="history-text">iOS, Android and the web</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">v3 pre-release</div><div class="history-text">one C ABI; what this workshop runs</div></div>
  <div class="history-item later upcoming"><div class="history-year">end of 11/2026</div><div class="history-name">v3.0</div><div class="history-text">the release</div></div>
</div>

Note:
- In July 2025 we hit the next limitation of anira.
- [pause] Until then anira only moved audio in and audio out, one block each.
- What was missing?
- The option to also process non-audio data, like features or predictions.
- So with v2, every input and output became a tensor.
- Each one has its own shape and its own rate.

---

<div class="tag">History</div>

## How anira evolved.<br><span class="then">From one thread per plugin to one C ABI.</span>

<div class="history">
  <div class="history-item"><div class="history-year">2023</div><div class="history-name">ADC23</div><div class="history-text">one inference thread per plugin</div></div>
  <div class="history-item"><div class="history-year">2024</div><div class="history-name">IS² paper</div><div class="history-text">anira, presented scientifically</div></div>
  <div class="history-item"><div class="history-year">2024</div><div class="history-name">ADC24</div><div class="history-text">anira v1 on stage</div></div>
  <div class="history-item"><div class="history-year">07/2025</div><div class="history-name">v2</div><div class="history-text">any tensor, not only audio</div></div>
  <div class="history-item current"><div class="history-year">2026</div><div class="history-name">Mobile · Web</div><div class="history-text">iOS, Android and the web</div></div>
  <div class="history-item later"><div class="history-year">2026</div><div class="history-name">v3 pre-release</div><div class="history-text">one C ABI; what this workshop runs</div></div>
  <div class="history-item later upcoming"><div class="history-year">end of 11/2026</div><div class="history-name">v3.0</div><div class="history-text">the release</div></div>
</div>

<div class="history-note">funded by the BMFTR since 2026</div>

Note:
- Earlier this year, we also ported the library to the web, since more and more audio processing moves there.
- [point at the note] Then we received public funding to develop anira further.
- With it came iOS and Android
- And the bigger goal: to support video, images and text, next to audio.
- v2 could not stretch that far, so we started a new design: v3.

---

<div class="tag">anira v3</div>

## Where v2 reached its limits.<br><span class="then">What v3 changes.</span>

<div class="limits">
  <div class="limit">
    {{ICON:model}}
    <div class="limit-v2"><span>v2</span>audio buffers, in float</div>
    <div class="limit-v3 fragment"><span>v3</span><div class="chips"><b>audio</b><b>image</b><b>text</b></div>typed tensors: ten dtypes, an image is a tensor too</div>
  </div>
  <div class="limit">
    {{ICON:code}}
    <div class="limit-v2"><span>v2</span>TypeScript wraps the C++ API</div>
    <div class="limit-v3 fragment"><span>v3</span><div class="chips"><b class="core">C ABI</b><i>→</i><b>C++</b><b>TypeScript</b></div>one versioned binary contract</div>
  </div>
  <div class="limit">
    {{ICON:clock}}
    <div class="limit-v2"><span>v2</span>built around the audio stream</div>
    <div class="limit-v3 fragment"><span>v3</span><div class="chips"><b>Hard · stream</b><b>Async · job</b></div>Hard today, Async later</div>
  </div>
  <div class="limit">
    {{ICON:chip}}
    <div class="limit-v2"><span>v2</span>data passes through CPU memory</div>
    <div class="limit-v3 fragment"><span>v3</span><div class="chips"><b>CPU</b><b>GPU</b><b>NPU</b></div>one tensor type on every device</div>
  </div>
</div>

Note:
- Why? v2 was built for audio plugins on the desktop, and that is where it reached its limits.
- [click] v3 now takes any kind data: audio, images and text.
- [click] We provide one clean C ABI at the core, with C++ and TypeScript on top of it.
- [click] Two contracts: Hard for a stream (what we do doay)
- But also support for Async for jobs, which is not streamed.
- [click] And we will support all tensor type. v2 only supports CPU, with v3 we will also support GPU and NPU. Which also means GPU and NPU inference!

---

<div class="tag">History</div>

## How anira evolved.<br><span class="then">From one thread per plugin to one C ABI.</span>

<div class="history">
  <div class="history-item"><div class="history-year">2023</div><div class="history-name">ADC23</div><div class="history-text">one inference thread per plugin</div></div>
  <div class="history-item"><div class="history-year">2024</div><div class="history-name">IS² paper</div><div class="history-text">anira, presented scientifically</div></div>
  <div class="history-item"><div class="history-year">2024</div><div class="history-name">ADC24</div><div class="history-text">anira v1 on stage</div></div>
  <div class="history-item"><div class="history-year">07/2025</div><div class="history-name">v2</div><div class="history-text">any tensor, not only audio</div></div>
  <div class="history-item"><div class="history-year">2026</div><div class="history-name">Mobile · Web</div><div class="history-text">iOS, Android and the web</div></div>
  <div class="history-item current"><div class="history-year">2026</div><div class="history-name">v3 pre-release</div><div class="history-text">one C ABI; what this workshop runs</div></div>
  <div class="history-item current upcoming"><div class="history-year">end of 11/2026</div><div class="history-name">v3.0</div><div class="history-text">the release</div></div>
</div>

<div class="history-note">Apache 2.0</div>

Note:
- Last stop: today you run v3.alpha3, pinned in the workshop's CMake.
- v3.0 is planned for the end of the month.
- That is the history; Now Fares, will talk about anira's inside.

