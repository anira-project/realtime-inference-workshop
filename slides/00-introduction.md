<h1>Real-Time Neural<br>Inference</h1>

From an exported neural model to a real-time-safe audio plugin

<!-- .slide: data-state="no-header" -->

---

## Who we are

<div class="people">
  <div class="person">
    <div class="person-photo person-initial">L</div>
    <div class="person-name">Lina</div>
    <div class="person-role">TODO</div>
  </div>
  <div class="person">
    <img class="person-photo" src="assets/images/people/fares.jpg" alt="Fares Schulz">
    <div class="person-name">Fares Schulz</div>
    <div class="person-role">anira · TU Berlin</div>
  </div>
  <div class="person">
    <img class="person-photo" src="assets/images/people/valentin.jpg" alt="Valentin Ackva">
    <div class="person-name">Valentin Ackva</div>
    <div class="person-role">anira · tanh-lab</div>
  </div>
</div>

Note:
    - TODO: Lina's surname, role and photo; check Fares' and Valentin's roles (taken from the ADC 2024 slides).

---

## Scope of this workshop

<div class="scope">
  <div class="scope-in">
    <h3>Today</h3>
    <ul>
      <li>An exported model, running in C++</li>
      <li>Host block size vs. model block size</li>
      <li>Worst-case timing</li>
      <li>Real-time safety</li>
      <li>Worker threads and latency</li>
      <li>A plugin — then anira</li>
    </ul>
  </div>
  <div class="scope-out">
    <h3>Not today</h3>
    <ul>
      <li>Training</li>
      <li>Exporting</li>
      <li>ML fundamentals</li>
      <li>Python</li>
    </ul>
  </div>
</div>

---

## Who this is for

- You write **real-time safe C++ DSP**: no allocations, no locks, no waiting on the audio thread
- Today: how to bring a **neural network** into that environment
- Neural network basics help, but are not required

---

## Follow along

<div class="qr-row">
  <div class="qr">
    {{QR:https://github.com/anira-project/realtime-inference-workshop}}
    <div class="qr-label">Repository</div>
    <div class="qr-url">github.com/anira-project/realtime-inference-workshop</div>
  </div>
  <div class="qr">
    {{QR:https://anira-project.github.io/realtime-inference-workshop/}}
    <div class="qr-label">Slides</div>
    <div class="qr-url">anira-project.github.io/realtime-inference-workshop</div>
  </div>
</div>

---

## Agenda

{{AGENDA}}

---

## How the exercises work

1. **Slides** — each step starts here: the problem, and the idea for fixing it
2. **`steps/NN_name/exercise/`** — your code, with TODOs to fill in
3. **`steps/NN_name/solution/`** — the finished step, to compare or to catch up

```plaintext
./build/bin/step01_exercise     your version
./build/bin/step01_solution     the reference
```

Letting an agent solve it gains you nothing — then read the solution directly.
