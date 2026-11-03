<!-- .slide: class="title-card" data-background-image="assets/images/adc26-title.webp" data-background-size="contain" data-state="no-chrome" -->

Note:
- When the session starts, go to the next slide.

---

<h1>Real-Time Neural<br>Inference</h1>

From an exported neural model to a real-time-safe audio plugin

Lina Campanella · Fares Schulz · Valentin Ackva

<div class="title-logos">
  <img class="title-logo bmftr" src="assets/images/logos/bmftr-funding.png" alt="With funding from the Federal Ministry of Research, Technology and Space">
  <img class="title-logo tu-berlin" src="assets/images/logos/tu-berlin.svg" alt="TU Berlin">
  <img class="title-logo baby-audio" src="assets/images/logos/baby-audio.svg" alt="Baby Audio">
</div>

<!-- .slide: data-state="no-header" -->

Note:
- Welcome to our workshop on real-time inference of neural networks.
- A neural network in a plugin has to deliver every block, on time, on every machine.
- Today we take an exported model all the way to an audio plugin that does exactly that.

---

<div class="tag">introduction</div>

## anira<br><span class="then">Research on real-time inference since 2020</span>

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
- This is anira, our open-source library for real-time inference.
- You give it a model for LibTorch, ONNX Runtime, LiteRT or ExecuTorch, and it takes care of the threads, the buffering and the latency.
- We have been working on real-time inference since 2020, and anira is what came out of it.
- It comes up throughout the workshop, so here is the short version.
- At ADC23 we showed how to run real-time inference in a plugin.
- At ADC24 we presented anira itself.
- We share these slides, so you can click through to both talks later.

---
<div class="tag">introduction</div>

## About us

<div class="people">
  <div class="person">
    <img class="person-photo" src="assets/images/people/lina.jpg" alt="Lina Campanella">
    <div class="person-name">Lina Campanella<span class="person-pronouns">she/her</span></div>
    <div class="person-role">Student Assistant</div>
    <div class="person-affiliation">TU Berlin</div>
  </div>
  <div class="person">
    <img class="person-photo" src="assets/images/people/fares.jpg" alt="Fares Schulz">
    <div class="person-name">Fares Schulz<span class="person-pronouns">he/him</span></div>
    <div class="person-role">Research Associate</div>
    <div class="person-affiliation">TU Berlin · tanh lab</div>
  </div>
  <div class="person">
    <img class="person-photo" src="assets/images/people/valentin.jpg" alt="Valentin Ackva">
    <div class="person-name">Valentin Ackva<span class="person-pronouns">he/him</span></div>
    <div class="person-role">Audio Software Developer</div>
    <div class="person-affiliation">Baby Audio · tanh lab</div>
  </div>
</div>

Note:
- So who are we?
- Lina is a student assistant at TU Berlin.
- I am a research associate at TU Berlin, and together with Valentin I run tanh lab.
- Valentin runs tanh lab with me and works as an audio software developer at Baby Audio.

---

<div class="tag">introduction</div>

## tanh lab<br><span class="then">From research to product.</span>

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

Note:
- Who builds and maintains anira?
- That is us, tanh lab, an audio software studio.
- We take ideas from research all the way to a product.
- On the research side, we design custom AI models that run in real time, for audio and video.
- On the product side, we build the plugins and mobile apps they run in, full stack.

---

<div class="tag">Scope</div>

## From an exported model<br>to real-time safe inference in a plugin.

<div class="cards spanned">
  <div class="card">{{ICON:model}}<div class="card-title">Exported model</div><div class="card-text">Trained, exported, given</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:chip}}<div class="card-title">Inference in C++</div><div class="card-text">Engines, block sizes, worst case</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:shield}}<div class="card-title">Real-time safe</div><div class="card-text">Worker thread, latency</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:plug}}<div class="card-title">Plugin</div><div class="card-text">All of it, in a host</div></div>
  <div class="cards-span" style="margin-bottom: 200px;"><span>anira</span> solves these three steps</div>
</div>


<div class="cards-note">Not today: training, exporting, ML fundamentals, Python.</div>

Note:
- Today's path: from an exported model to real-time safe inference in a plugin.
- We start from a model that is already trained and exported.
- We run it in C++, and see what the engine, the block size and the worst case mean for us.
- We make it real-time safe, with a worker thread and an honest latency.
- And we put all of it into a plugin that runs in your host.
- anira solves these three steps, and today you build them by hand first.
- Not today: training, exporting, machine learning basics or Python.

---


## Who this is for<br><span class="then">We assume you write real-time audio code in C++.</span>

<div class="prereqs">
  <div class="prereq">
    <div class="prereq-label">Expected knowledge</div>
    <div class="prereq-text">C++, and the audio thread's rules</div>
  </div>
  <div class="prereq optional">
    <div class="prereq-label">Helpful knowledge, not required</div>
    <div class="prereq-text">Neural network basics</div>
  </div>
</div>

Note:
- Who is this for?
- We assume you write real-time audio code in C++.
- You know the rules of the audio thread: no allocations, no locks, no waiting.
- Neural network basics help, but you do not need them.

---

## Follow along

<div class="qr-row">
  <div class="qr">
    {{QR:HTTPS://GITHUB.COM/ANIRA-PROJECT/REALTIME-INFERENCE-WORKSHOP}}
    <div class="qr-label">Repository</div>
    <div class="qr-url">github.com/anira-project/realtime-inference-workshop</div>
  </div>
</div>

Note:
- Follow along: everything we show is in this repository.
- Please scan the code now, or type the address.
- The slides are linked there too, top right on the GitHub page.
- If you cannot read the screen well, click through them on your own machine.
- If you have not built the code yet, clone it and start the configure step now, so it runs while we talk.

---

## Agenda

{{AGENDA}}

Note:
- The workshop has two parts.
- In the first part we build real-time inference by hand, step by step, up to a plugin.
- In the second part we replace the hand-built parts with anira.

---

<div class="tag">explanation</div>

## How the exercises work <br>

<span class="then">Each step: a short introduction, then an exercise.<br>A reference solution is in the repository.</span>

<div class="cards" style="margin-bottom: 200px;"> 
  <div class="card">{{ICON:slides}}<div class="card-title">Slides</div><div class="card-text">The problem, and the idea for fixing it</div></div>
  <div class="card-arrow">→</div>
  <div class="card accent">{{ICON:code}}<div class="card-title"><code>exercise/</code></div><div class="card-text">Your code, with TODOs to fill in</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:folder}}<div class="card-title"><code>solution/</code></div><div class="card-text">To compare, or to catch up</div></div>
</div>

Note:
- How do the exercises work?
- Every step follows the same pattern.
- On the slides we show the problem and the idea for fixing it.
- Then you fill in the TODOs in exercise/.
- Each exercise tells you which TODO is still open when you run it.
- If you get stuck, or want to compare, the solution is in solution/.

