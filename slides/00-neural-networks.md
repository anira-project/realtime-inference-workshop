<h1>Neural networks<br>in real-time audio</h1>

What we are integrating today, and where it comes from

<!-- .slide: data-state="no-header" -->
<!-- part: Real-time inference -->

---

## Audio → audio

<div class="nn-flow">
  <div class="nn-node">audio<small>a block of samples</small></div>
  <div class="nn-arrow">→</div>
  <div class="nn-node model">model</div>
  <div class="nn-arrow">→</div>
  <div class="nn-node">audio<small>a block of samples</small></div>
</div>

- **RAVE** — timbre transfer, **Demucs** — source separation, neural amp and effect models
- Runs on every block, at audio rate: the model *is* the effect
- Its latency is the plugin's latency

---

## Parameters → audio

<div class="nn-flow">
  <div class="nn-node">parameters<small>pitch, loudness, text, …</small></div>
  <div class="nn-arrow">→</div>
  <div class="nn-node model">model</div>
  <div class="nn-arrow">→</div>
  <div class="nn-node">audio<small>a block of samples</small></div>
</div>

- **DDSP** — pitch and loudness in, an instrument out, **text-to-speech**
- Control rate in, audio rate out: the model *is* the sound generator
- Has to deliver every block, whether the input changed or not

---

## Audio → parameters

<div class="nn-flow">
  <div class="nn-node">audio<small>a block of samples</small></div>
  <div class="nn-arrow">→</div>
  <div class="nn-node model">model</div>
  <div class="nn-arrow">→</div>
  <div class="nn-node">parameters<small>pitch, text, events, …</small></div>
</div>

- **PESTO**, **CREPE** — pitch estimation, **speech-to-text**
- Audio rate in, control rate out: the model *listens*
- The result often leaves the audio thread — to the UI, to MIDI, to a file

---

## Stateless vs. stateful

<div class="nn-compare">
  <div>
    <h3>Stateless</h3>
    <p>The output depends only on this call's input.</p>
    <ul>
      <li>Context has to come with the input, e.g. overlapping windows</li>
      <li>Calls are independent of each other</li>
    </ul>
  </div>
  <div>
    <h3>Stateful</h3>
    <p>The model carries memory from one call to the next.</p>
    <ul>
      <li>RNN hidden states, cached convolution history</li>
      <li>Calls must come in order, one state per stream</li>
      <li>The state has to be reset when the stream restarts</li>
    </ul>
  </div>
</div>

Ours is **stateful**: 40 state tensors, carried from block to block.

---

## And many variations

- **Conditioned**: audio and parameters → audio, e.g. an amp model with a gain knob
- **Causal or not**: a model that looks into the future needs that future as latency
- **Fixed or free input size**: a model traced for 2048 samples takes exactly 2048
- **Rates**: audio rate, control rate, or one result per note

The type decides where the model runs, how often, and what the host waits for.

---

## Our model

<div class="nn-flow">
  <div class="nn-node">audio<small>2048 samples</small></div>
  <div class="nn-arrow">→</div>
  <div class="nn-node model">encoder</div>
  <div class="nn-arrow">→</div>
  <div class="nn-node latent">z<small>16 numbers</small></div>
  <div class="nn-arrow">→</div>
  <div class="nn-node model">decoder</div>
  <div class="nn-arrow">→</div>
  <div class="nn-node">audio<small>2048 samples</small></div>
</div>

- A **variational autoencoder**, audio → audio, from `seqsynth`
- 48 kHz, mono, 2048 samples per call: 42.7 ms of audio squeezed into 16 numbers
- Streaming and **stateful**, 65 MB of weights

Note:
    - TODO: confirm the architecture details with Fares (PQMF front end, encoder/decoder layout).

---

## How it was trained

1. The **encoder** maps the audio to a distribution over `z`
2. A `z` is drawn from it, the **decoder** turns it back into audio
3. The loss: **reconstruction** — how close is the output to the input — plus a term
   that keeps `z` close to a normal distribution

Afterwards: a PCA over the latents orders the 16 dimensions by how much they matter.

TODO: dataset, losses, training time (checkpoint: 625,000 steps)

Note:
    - The PCA is whitened; the 16 components are ordered by explained variance (11.6 % for the first, 2.5 % for the last).

---

## What it is for

The 16 numbers are a handle on the sound: change them, and the decoder makes something new.

<div class="audio-demo">
  <div>
    <h3>In</h3>
    <audio controls src="assets/audio/demo_input.wav"></audio>
  </div>
  <div>
    <h3>Out — through encoder and decoder</h3>
    <audio controls src="assets/audio/demo_output.wav"></audio>
  </div>
</div>

Rendered block by block, 2048 samples at a time — exactly what we will do in C++.

Note:
    - Rendered with scripts/render_demo.py; the input is synthesized there.

---

## Neural network integration

<div class="integration">
  <div class="integration-step">
    <svg viewBox="0 0 200 140" aria-hidden="true">
      <g class="integration-lines">
        <line x1="30" y1="40" x2="80" y2="25"/><line x1="30" y1="40" x2="80" y2="70"/><line x1="30" y1="40" x2="80" y2="115"/>
        <line x1="30" y1="100" x2="80" y2="25"/><line x1="30" y1="100" x2="80" y2="70"/><line x1="30" y1="100" x2="80" y2="115"/>
        <line x1="80" y1="25" x2="130" y2="45"/><line x1="80" y1="70" x2="130" y2="45"/><line x1="80" y1="115" x2="130" y2="45"/>
        <line x1="80" y1="25" x2="130" y2="95"/><line x1="80" y1="70" x2="130" y2="95"/><line x1="80" y1="115" x2="130" y2="95"/>
        <line x1="130" y1="45" x2="175" y2="70"/><line x1="130" y1="95" x2="175" y2="70"/>
      </g>
      <g class="integration-dots">
        <circle cx="30" cy="40" r="8"/><circle cx="30" cy="100" r="8"/>
        <circle cx="80" cy="25" r="8"/><circle cx="80" cy="70" r="8"/><circle cx="80" cy="115" r="8"/>
        <circle cx="130" cy="45" r="8"/><circle cx="130" cy="95" r="8"/><circle cx="175" cy="70" r="8"/>
      </g>
    </svg>
    <div class="integration-label">Train</div>
    <div class="integration-title">Train the network</div>
    <div class="integration-detail">in PyTorch</div>
    <div class="integration-language">Python</div>
  </div>
  <div class="integration-step">
    <svg viewBox="0 0 200 140" aria-hidden="true">
      <circle class="integration-ring" cx="100" cy="70" r="58"/>
      <circle class="integration-ring accent" cx="100" cy="70" r="46"/>
    </svg>
    <div class="integration-label">Export</div>
    <div class="integration-title">Export the model</div>
    <div class="integration-detail">TorchScript <code>.pt</code>, ONNX</div>
    <div class="integration-language">Python</div>
  </div>
  <div class="integration-step today">
    <svg viewBox="0 0 200 140" aria-hidden="true">
      <path class="integration-curve" d="M10 130 C 90 120, 110 20, 190 15"/>
      <path class="integration-curve accent" d="M10 130 C 90 125, 120 50, 190 45"/>
      <path class="integration-curve" d="M10 130 C 90 130, 130 80, 190 75"/>
    </svg>
    <div class="integration-label">Implement</div>
    <div class="integration-title">Run it in the audio environment</div>
    <div class="integration-detail">LibTorch, ONNX Runtime — real-time safe</div>
    <div class="integration-language">C++</div>
  </div>
</div>

The first two are done. **Today starts at the third.**

Note:
    - From the ADC 2024 talk (Real-Time Inference of Neural Networks, Part II).
