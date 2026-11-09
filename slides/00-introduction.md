<!-- .slide: class="title-card" data-background-image="assets/images/adc26-title.webp" data-background-size="contain" data-state="no-chrome" -->

Note:
- When the session starts, go to the next slide.

---

<h1>Real-Time Neural<br>Inference</h1>

From an exported neural model to a real-time-safe audio plugin

Lina Campanella · Fares Schulz · Valentin Ackva

<div class="title-logos">
  <img class="title-logo bmftr" src="assets/images/logos/bmftr-funding.png" alt="With funding from the Federal Ministry of Research, Technology and Space">
</div>

<!-- .slide: data-state="no-header" -->

Note:
- Hello everyone,
- and welcome to our workshop on real-time inference of neural networks.
- Today we take a neural network that has already been trained and exported, and step by step turn it into a real-time safe audio plugin.

---

<div class="tag">Teaser · A</div>

## Every callback has a deadline<br><span class="then">An FFT on 2048 samples: cheap, and equally cheap every time.</span>

<div class="teaser" data-teaser="clock" data-work="fft"><canvas></canvas></div>

Note:
- But before we talk about neural networks, let's first look at something familiar:
- Fixed block FFT processing within the host's audio callback.
- For example we can have a host audio callback with 512 samples and a fixed size FFT processing module which requires 2048 samples.
- How is this typically implemented?
- You'd use a ring buffer to collect samples for 3 callbacks, then in the fourth you can run your FFT processing
- On this machine it takes less than the equivalent of a sample per block constantly
- And then the samples are returned
- So we will have a little processing spike every 4 host callbacks, but since we are so quick it does not matter much

---

<div class="tag">Teaser · B</div>

## Every callback has a deadline<br><span class="then">A network on 2048 samples: too slow, and its time varies.</span>

<div class="teaser" data-teaser="clock" data-work="nn"><canvas></canvas></div>

Note:
- If we replace the FFT module with a neural network module, we have similarities
- We also have a fixed input size for the neural network (here 2048 as well), but the processing takes longer
- And on top of that the processing is nondeterministic, it varies in time
- Today's question is therefore: How can we fit a fixed-size, non-deterministic model into a callback with varying block sizes and a hard deadline?

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
- The short answer: you can use our anira library.
- It is an open-source library for real-time inference.
- You give it a model for LibTorch, ONNX Runtime, LiteRT or ExecuTorch, and it takes care of the threads, the buffering and the latency.
- It is the outcome of our work and research about real-time inference since 2020.
- If you want you can also look at our earlier implementation proposals, which we presented at ADC23 and ADC24.

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
    <div class="person-role">Research Associate & Freelance Software Developer</div>
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
- But before diving deeper into the topic let us first introduce ourselves
- Lina Campanella is working as a student assistant at TU Berlin.
- I am a research associate at TU Berlin leading the Computer Music and Neural Audio Systems research team, and together with Valentin I run tanh lab.
- In addition Valentin also works as an audio software developer at Baby Audio.

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
- What is tanh lab?
- We’re an audio software studio based in Berlin
- We take ideas from research all the way to a product.
- Sometimes that means applied research, where we find out whether an idea is actually feasible.
- Sometimes it means delivering the complete product, from the DSP core to deployment.
- Our focus is real-time audio and machine learning and ... especially where the two meet: getting neural networks to run reliably inside an audio callback.
- That’s also why we maintain anira, an open-source, permissively licensed C++ library for real-time neural network inference in audio.

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
  <div class="cards-span" style="margin-bottom: 200px;"><span>Workshop Scope</span> How to solve these three steps</div>
</div>


<div class="cards-note">Not today: training, exporting, ML fundamentals, Python.</div>

Note:
- Now let's talk about the workshop.
- Today's path starts from an exported model (get it from a data scientist) already trained and exported.
- We will run this model in C++, and see what the engine, the block size and the worst case mean for us.
- We make it real-time safe, with a worker thread and an honest latency.
- And we put all of it into a plugin that runs in your host.
- anira solves these three steps, but in order to understand why it is useful, today you will build it by hand first.
- So today we are not going to cover: training, exporting, machine learning basics or Python.

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
- If you have not done so already, please clone the repo and start the configure step now, so it runs while we talk.

---

<div class="tag">Setup</div>

## Wi-Fi<br><span class="then">Connect before you clone.</span>

<div class="prereqs">
  <div class="prereq">
    <div class="prereq-label">Network</div>
    <div class="prereq-text">WIFI_NAME</div>
  </div>
  <div class="prereq">
    <div class="prereq-label">Password</div>
    <div class="prereq-text"><code>audiodevcon</code></div>
  </div>
</div>

Note:
- Before the build: the Wi-Fi.
- The network and password are on the slide.
- The network is shared, so please connect now and start the download right away.

---

<div class="tag">Setup</div>

## Set up now<br><span class="then">Clone, configure and build while we talk.</span>

```bash
git clone https://github.com/anira-project/realtime-inference-workshop.git
cd realtime-inference-workshop
cmake --preset release           # about 350 MB: models, ONNX Runtime, JUCE, anira
cmake --build --preset release   # libraries and solutions
```

<div class="cards-note">Release, because we benchmark the model later.</div>

<div class="cards-note">Cloned it at home already? <code>git pull</code> and configure again: only what changed is fetched.</div>

Note:
- We sent the setup instructions by email. If you did it at home: thank you, just pull and configure again.
- If not: please start now. Clone, configure, build.
- The instructions are also in our workshop discord channel or the github README
- Configuring downloads about 350 MB: the models, ONNX Runtime, JUCE and anira. The room's network is shared, so the earlier the better.
- Ideally the build is then finished in time for the first exercise
- Why release? There is a benchmark in the examples, which is only meaningful in release.
- Debugging works too, But it fetches the dependencies a second time, so please do it at another moment, not now on the shared network.
- If configuring fails, raise your hand and we come to you.
- Again .. please! Do not wait for the first exercise.

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
- You can check if you got the right solution by running the exercise.
- If you get stuck, or want to compare, the solution is in solution/.

