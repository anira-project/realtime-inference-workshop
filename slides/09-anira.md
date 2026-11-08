<h1>anira</h1>

Replacing the hand-built parts

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->
<!-- part: anira -->

Note:
- Welcome back.
- Lina said it before the break: you'd write all of it again for every new model.
- That is anira's job.
- What it is, where it comes from; then Fares shows the inside; then you build with it.

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
- Here is part 1 in one picture.
- Inference: the model's state, carried by hand.
- Benchmarking: twenty passes on your laptop, and a number typed into a header.
- Real-time safety: run once, on the step 5 demo, never on your plugin.
- Threading: and what happens when one session runs eight instances?
- Minimal latency: right only for a host block of 512.
- Real-time guarantees: that header number, `k_max_inference_ms` [say: k-max-inference-m-s], is the budget on every user's machine too.
- Nobody measures there, and a miss becomes a dropout.
- [pause, then click] Step 9, your first anira exercise, shows you how, on this same picture.
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
- You write two files: model.json describes the model, contract.json how it runs.
- The contract is what the host promises and what one inference may cost: Fares goes through it.
- From those, anira does what you built in steps 3 to 7.
- Underneath is one C ABI [say: C, A-B-I]: plain exported C functions, with C++ and TypeScript on top.
- [click] Four built-in engines or your own; today we use ONNX Runtime, the engine of part 1.
- LiteRT is TensorFlow Lite's new name: one runtime, two APIs, and anira builds with one of them.
- ExecuTorch is the PyTorch runtime for phones and boards.
- [click] It runs on all of these, from the browser via WebAssembly to a Bela board.
- [click] Of the hosts, the last one, real-time video, is your step 11: a hand tracker.

---

<div class="tag">In production</div>

## anira in products.<br><span class="then">A selection.</span>

<div class="cards shipped">
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/drum-gate-2.jpg" alt=""></div><figcaption><span class="card-company">Sonnox · Plugin</span>Oxford Drum Gate 2</figcaption></figure>
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/subculture.jpg" alt=""></div><figcaption><span class="card-company">Baby Audio · Plugin</span>SubCulture</figcaption></figure>
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/lydia.jpg" alt=""></div><figcaption><span class="card-company">Neutone · Embedded</span>Project LYDIA</figcaption></figure>
</div>

Note:
- anira started as a research project, and today it ships in released products.
- [click] Sonnox Oxford Drum Gate 2 classifies drum hits by their timbre.
- [click] Baby Audio SubCulture, a pitch-tracking bass enhancer.
- [click] And Project LYDIA by Neutone, a pedal on a Raspberry Pi 5.
- So how did we get here: not in one go.

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
- The timeline runs from 2023 to the end of this month, and we stop five times.
- First stop, ADC23: we showed a plugin template.
- It is the design you built in part 1: a worker, a ring buffer each way, a prefill.
- It worked, for one plugin.
- [click] After the talk the sonible team asked: have you thought about many instances?
- [pause, let them read the second line] A fair question, so let's look at what it means.

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
- [ask the room, hands up] Who runs eight or more instances of one plugin in a session?
- Then this picture is yours.
- Each thread alone is fine, but together there are more real-time threads than cores.
- They compete with each other and with the host's audio thread, so one waits and its block comes late.
- [pause] Step 4 taught you the worst case decides, and it grows with every instance.
- [click] So the fix has to live one level up, in the library.
- The pool is sized by the cores, not by the instances: eight plugins, and still no more threads than cores.
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
- Back on the timeline, second stop: 2024, two entries.
- First the paper at IS² [say: I-S-two], the Internet of Sounds symposium, where it won the Industry Award.
- Then, in November, ADC24: anira v1 on stage, with the shared pool you just saw.

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
- July 2025, v2.
- [pause] Until then anira moved audio in and audio out, one block each.
- With v2 a model can take control inputs next to the audio, like pitch and loudness.
- Those come slower: a few values per block, not one per sample.
- So every input and output became a tensor, with its own shape and its own rate.
- Still float, and still no phone and no browser: the next two stops.

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
- 2026, and anira reaches phones and the browser.
- In June came the browser, anira compiled to WebAssembly.
- Nine days later, iOS and Android, still on v2.
- [point at the note] anira is now publicly funded, [pause] by the German Federal Ministry of Research, Technology and Space.
- The funding has a goal: video, images and text, not only audio.
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
- Four limits, four changes.
- [click] Why ten dtypes [say: d-types]: an image comes as eight-bit integers, text as token IDs.
- [click] The C ABI from the overview: both languages now sit on the same C functions.
- [click] Two kinds of contract now, both written in contract.json.
- Hard is the stream you know: blocks in and out, every callback.
- Async is one job without a callback, an image for example.
- [click] One tensor type that says where its memory lives, on the CPU or a GPU.
- Today every tensor lives in CPU memory.
- GPU and NPU, your laptop's neural engine, are the outlook chapter's topic.

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
- Last stop: today you run alpha 3, pinned in the workshop's CMake.
- v3.0 is planned for the end of the month.
- The licence is Apache 2.0: free to use in commercial products, like the three you saw.
- That is the history; Fares, over to you for the inside.

