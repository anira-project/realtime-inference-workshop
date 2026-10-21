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
  <div class="needs-anira fragment"><span>anira</span> has an answer to all six, built for production: from one plugin instance to many.</div>
</div>

Note:
    - What we did in part 1, as six needs. Every one works, and every one is the simple version: one engine, one machine, one instance.
    - Click once: anira covers all six, from years of plugins in production. Not how yet; the next two chapters open them one by one.
    - The dry path is the one thing that stays in your code: anira reports the number, the mix is the plugin's.

---

<div class="tag">What anira is</div>

## A C++ library for real-time inference.<br><span class="then">Configure once, run everywhere.</span>

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

## Battle-tested in the industry.<br><span class="then">Two plugins on sale, one hardware project.</span>

<div class="cards shipped">
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/drum-gate-2.jpg" alt=""></div><figcaption><span class="card-company">Sonnox · Plugin</span>Oxford Drum Gate 2</figcaption></figure>
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/subculture.jpg" alt=""></div><figcaption><span class="card-company">Baby Audio · Plugin</span>SubCulture</figcaption></figure>
  <figure class="fragment"><div class="card-photo"><img src="assets/images/products/lydia.jpg" alt=""></div><figcaption><span class="card-company">Neutone · Embedded</span>Project LYDIA</figcaption></figure>
</div>

Note:
    - Sonnox Oxford Drum Gate 2 (February 2026): the gate classifies drum hits by timbre, not level. Its models run through anira.
    - Baby Audio SubCulture (July 2026): pitch-tracking bass enhancer; its models run through anira.
    - Project LYDIA by Neutone: Neutone's Morpho on a Raspberry Pi 5, the embedded case from the slide before. A hardware project, not a product on sale yet.
    - Two commercial plugins on every desktop host, and a pedal on ARM: the same library in all three.
    - Images: the Sonnox hero shot and the Baby Audio screenshot from their product pages, the pedal from Neutone.

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

<div class="cards-note">Two JSON files and three calls. The rest is the library's. Today: v3.0.0-beta, pinned by CMake.</div>

Note:
    - ModelConfig: which files, which engines may run them, every tensor with a name and a role (streamed, static, state).
    - Contract: what the host promises and what it gets. Hard: a block size range, a time budget, what happens on a miss; the latency follows from it. Async: a job with a deadline, for things like a camera frame. Everything in the hands-on is Hard, except the hand tracking in step 11.
    - Version: the workshop pins the v3 beta tag; CMake fetches it with ONNX Runtime built in.
    - ContextConfig: the process-wide part, threads and enabled engines. One per process, like the pool.
    - Handler: the three calls from step 8, prepareToPlay, processBlock and setLatencySamples, one each.

---

<div class="tag">Why anira</div>

## One plugin, two threads.<br><span class="then">Ten plugins, twenty threads, eight cores.</span>

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

Note:
    - Why it exists at all. Step 8 ended here: one engine and one worker per channel, per instance. The host does not know, and keeps adding instances.
    - Real-time threads that outnumber cores take turns. A worker that is not scheduled misses its block, and the miss lands in someone else's plugin.
    - This is the problem anira was written for. Everything else followed from it.

---

<div class="tag">History</div>

## Written for that problem,<br><span class="then">in 2024.</span>

<div class="history">
  <div class="history-item fragment"><div class="history-year">2023</div><div class="history-name">ADC23</div><div class="history-text">one inference thread per plugin</div></div>
  <div class="history-item fragment"><div class="history-year">2024</div><div class="history-name">anira · ADC24</div><div class="history-text">one pool per process; IS² paper</div></div>
  <div class="history-item fragment"><div class="history-year">11/2024</div><div class="history-name">v1</div><div class="history-text">context, multichannel</div></div>
  <div class="history-item fragment"><div class="history-year">07/2025</div><div class="history-name">v2</div><div class="history-text">any tensor, not only audio</div></div>
  <div class="history-item fragment"><div class="history-year">2026</div><div class="history-name">v2.1 – v2.3</div><div class="history-text">web, mobile, generators</div></div>
  <div class="history-item fragment"><div class="history-year">2026</div><div class="history-name">v3</div><div class="history-text">one C ABI, every host</div></div>
</div>

<div class="history-note">Apache 2.0 · tanh lab, TU Berlin · funded by BMFTR since 2026</div>

Note:
    - ADC23 (London): the nn-inference-template, an inference thread per plugin, and the oversubscription slide at the end.
    - ADC24 (Bristol): anira, with the static thread pool, and the IEEE IS² 2024 paper, which won the Industry Award.
    - v1: anira::Context, one pool shared by every handler; multichannel. v2: arbitrary tensors, non-audio inputs and outputs, different rates in and out.
    - 2026: web (WASM), Android and iOS, LiteRT and ExecuTorch, generators and analysers, a closed-form latency. Then v3: a versioned C ABI, so every host and language sees the same library.
    - Since 2026 funded by the BMFTR through the Prototype Fund; Lina joined the team.

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
