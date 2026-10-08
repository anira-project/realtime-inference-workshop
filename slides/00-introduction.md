<h1>Real-Time Neural<br>Inference</h1>

From an exported neural model to a real-time-safe audio plugin

Lina Campanella · Fares Schulz · Valentin Ackva

<!-- .slide: data-state="no-header" -->

---

<div class="tag">anira</div>

## Research on real-time inference since 2020,<br><span class="then">implemented in the open-source library anira (v3).</span>

<div class="brand anira">
  <img class="brand-mark" src="assets/images/logos/anira-mark.png" alt="anira logo">
  <ul class="checks">
    <li>Commercially usable: Apache 2.0</li>
    <li>Any inference engine: LibTorch, ONNX Runtime, LiteRT, ExecuTorch</li>
    <li>Real-time safe, with minimal latency</li>
  </ul>
  <div class="row-label">Platforms:</div>
  <div class="platforms">
    <figure><img src="assets/images/platforms/apple.svg" alt=""><figcaption>macOS</figcaption></figure>
    <figure><img src="assets/images/platforms/windows.svg" alt=""><figcaption>Windows</figcaption></figure>
    <figure><img src="assets/images/platforms/linux.svg" alt=""><figcaption>Linux</figcaption></figure>
    <figure><img src="assets/images/platforms/ios.svg" alt=""><figcaption>iOS</figcaption></figure>
    <figure><img src="assets/images/platforms/android.svg" alt=""><figcaption>Android</figcaption></figure>
    <figure><img src="assets/images/platforms/webassembly.svg" alt=""><figcaption>Browser</figcaption></figure>
  </div>
  <div class="row-label">Prev. talks:</div>
  <div class="talks">
    <a class="talk" href="https://www.youtube.com/watch?v=z_RKgHU59r0" target="_blank"><span class="talk-event">ADC23</span>Real-Time Inference of Neural Networks: A Guide for DSP Engineers</a>
    <a class="talk" href="https://www.youtube.com/watch?v=p-WwttPZJ_o" target="_blank"><span class="talk-event">ADC24</span>Real-Time Inference of Neural Networks, Part II</a>
  </div>
</div>

Note:
    - Version 3 is out as v3.0.0-alpha.1 (September 2026); v2.3.0 is the latest stable release.
    - Most of what the workshop shows comes from building anira.

---

## Who we are

<div class="people">
  <div class="person">
    <img class="person-photo" src="assets/images/people/lina.jpg" alt="Lina Campanella">
    <div class="person-name">Lina Campanella<span class="person-pronouns">she/her</span></div>
    <div class="person-role">Researcher</div>
    <div class="person-affiliation">TU Berlin</div>
  </div>
  <div class="person">
    <img class="person-photo" src="assets/images/people/fares.jpg" alt="Fares Schulz">
    <div class="person-name">Fares Schulz<span class="person-pronouns">he/him</span></div>
    <div class="person-role">Senior Researcher</div>
    <div class="person-affiliation">TU Berlin · tanh lab</div>
  </div>
  <div class="person">
    <img class="person-photo" src="assets/images/people/valentin.jpg" alt="Valentin Ackva">
    <div class="person-name">Valentin Ackva<span class="person-pronouns">he/him</span></div>
    <div class="person-role">Audio Software Developer</div>
    <div class="person-affiliation">Baby Audio · tanh lab</div>
  </div>
</div>

---

<div class="tag">tanh lab</div>

## An audio software studio.<br><span class="then">From research to product.</span>

<div class="brand studio">
  <img class="brand-lockup" src="assets/images/logos/tanh-lab.svg" alt="tanh lab">
  <div class="brand-body">
    <div class="studio-rows">
      <div class="studio-row">{{ICON:chip}}<div class="card-title">Research</div><ul class="card-points"><li>DSP and machine learning</li><li>Feasibility studies, prototypes</li></ul></div>
      <div class="studio-row">{{ICON:model}}<div class="card-title">Custom models</div><ul class="card-points"><li>Neural networks for audio</li><li>On device, in real time</li></ul></div>
      <div class="studio-row">{{ICON:plug}}<div class="card-title">Full stack</div><ul class="card-points"><li>Plug-ins, apps, mobile, web</li><li>From method to product</li></ul></div>
    </div>
    <div class="cards-note">tanh-lab.com · we develop and maintain anira</div>
  </div>
</div>

---

<div class="tag">Scope</div>

## From an exported model<br><span class="then">to real-time safe inference in a plugin.</span>

<div class="cards spanned">
  <div class="card">{{ICON:model}}<div class="card-title">Exported model</div><div class="card-text">Trained, exported, given</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:chip}}<div class="card-title">Inference in C++</div><div class="card-text">Engines, block sizes, worst case</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:shield}}<div class="card-title">Real-time safe</div><div class="card-text">Worker thread, latency</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:plug}}<div class="card-title">Plugin</div><div class="card-text">All of it, in a host</div></div>
  <div class="cards-span"><span>anira</span> solves all three</div>
</div>

<div class="cards-note">Not today: training, exporting, ML fundamentals, Python.</div>

---

<div class="tag">Who this is for</div>

## We assume you write real-time audio code in C++.<br><span class="then">We add neural network inference to it.</span>

<div class="prereqs">
  <div class="prereq">
    <div class="prereq-label">You should know</div>
    <div class="prereq-text">C++, and the audio thread's rules: no allocations, no locks, no waiting</div>
  </div>
  <div class="prereq optional">
    <div class="prereq-label">Helpful knowledge, not required</div>
    <div class="prereq-text">Neural network basics</div>
  </div>
</div>

---

## Follow along

<div class="qr-row">
  <div class="qr">
    {{QR:HTTPS://GITHUB.COM/ANIRA-PROJECT/REALTIME-INFERENCE-WORKSHOP}}
    <div class="qr-label">Repository</div>
    <div class="qr-url">github.com/anira-project/realtime-inference-workshop</div>
  </div>
</div>

---

## Agenda

{{AGENDA}}

---

<div class="tag">How the exercises work</div>

## Each step: a short introduction, then an exercise.<br><span class="then">A reference solution is in the repository.</span>

<div class="cards">
  <div class="card">{{ICON:slides}}<div class="card-title">Slides</div><div class="card-text">The problem, and the idea for fixing it</div></div>
  <div class="card-arrow">→</div>
  <div class="card accent">{{ICON:code}}<div class="card-title"><code>exercise/</code></div><div class="card-text">Your code, with TODOs to fill in</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:folder}}<div class="card-title"><code>solution/</code></div><div class="card-text">To compare, or to catch up</div></div>
</div>

