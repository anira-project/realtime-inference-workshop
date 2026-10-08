<h1>Neural networks in<br>real-time environments</h1>

What we are integrating today, and where it comes from

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->
<!-- part: Real-time inference -->

---

## Neural network integration

{{INTEGRATION:train}}

First: where the model comes from.

---

## Neural networks in real-time audio

<div class="nn-types">
  <div class="nn-type">
    <div class="nn-node">audio</div><div class="nn-arrow">→</div>
    <div class="nn-node model">model</div><div class="nn-arrow">→</div>
    <div class="nn-node">audio</div>
    <div class="nn-examples">RAVE · Demucs · amp models</div>
  </div>
  <div class="nn-type">
    <div class="nn-node">parameters</div><div class="nn-arrow">→</div>
    <div class="nn-node model">model</div><div class="nn-arrow">→</div>
    <div class="nn-node">audio</div>
    <div class="nn-examples">DDSP · text-to-speech</div>
  </div>
  <div class="nn-type">
    <div class="nn-node">audio</div><div class="nn-arrow">→</div>
    <div class="nn-node model">model</div><div class="nn-arrow">→</div>
    <div class="nn-node">parameters</div>
    <div class="nn-examples">PESTO · CREPE · speech-to-text</div>
  </div>
</div>

Note:
    - Audio → audio: runs every block at audio rate, the model is the effect; its latency is the plugin's latency.
    - Parameters → audio: control rate in, audio out, the model is the sound generator; has to deliver every block.
    - Audio → parameters: the model listens; the result often leaves the audio thread (UI, MIDI, file).

---

## Stateless vs. stateful

<div class="state-demo">
  <div class="state-label">Stateless</div>
  <div class="state-row">
    <div class="nn-node">call 1</div><div class="state-gap"></div>
    <div class="nn-node">call 2</div><div class="state-gap"></div>
    <div class="nn-node">call 3</div>
  </div>
  <div class="state-label">Stateful</div>
  <div class="state-row">
    <div class="nn-node model">call 1</div><div class="state-link">state →</div>
    <div class="nn-node model">call 2</div><div class="state-link">state →</div>
    <div class="nn-node model">call 3</div>
  </div>
</div>

Ours is **stateful**: 40 state tensors, carried from block to block.

Note:
    - Stateless: output depends only on this call's input; context has to come with the input (overlapping windows); calls are independent.
    - Stateful: memory from one call to the next (RNN hidden states, cached convolution history); calls in order, one state per stream, reset when the stream restarts.

---

## And many variations

<div class="tiles-grid variations">
  <div class="tile"><h3>Conditioned</h3><div class="tile-description">audio + knobs → audio</div></div>
  <div class="tile"><h3>Causal or not</h3><div class="tile-description">the future costs latency</div></div>
  <div class="tile"><h3>Fixed input size</h3><div class="tile-description">traced for 2048, takes 2048</div></div>
  <div class="tile"><h3>Rates</h3><div class="tile-description">audio, control, per note</div></div>
</div>

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

<div class="facts">
  <span>variational autoencoder</span><span>48 kHz mono</span><span>2048 samples per call</span><span>stateful</span><span>65 MB</span>
</div>

Note:
    - From seqsynth. 42.7 ms of audio squeezed into 16 numbers.
    - TODO: confirm the architecture details with Fares (PQMF front end, encoder/decoder layout).

---

## How it was trained

<div class="nn-flow">
  <div class="nn-node">audio</div>
  <div class="nn-arrow">→</div>
  <div class="nn-node model">encoder</div>
  <div class="nn-arrow">→</div>
  <div class="nn-node latent">z<small>drawn from 𝒩(μ, σ)</small></div>
  <div class="nn-arrow">→</div>
  <div class="nn-node model">decoder</div>
  <div class="nn-arrow">→</div>
  <div class="nn-node">audio′</div>
</div>

<div class="facts">
  <span>loss: audio′ ≈ audio</span><span>+ z ≈ 𝒩(0, 1)</span><span>then PCA: 16 dims, by importance</span>
</div>

TODO: dataset, losses, training time (checkpoint: 625,000 steps)

Note:
    - Encoder maps the audio to a distribution over z; a z is drawn, the decoder turns it back into audio.
    - Loss: reconstruction plus a term that keeps z close to a normal distribution.
    - The PCA is whitened; the 16 components are ordered by explained variance (11.6 % for the first, 2.5 % for the last).

---

## What it is for

<div class="spectro-player" data-a="assets/audio/demo_input.wav" data-b="assets/audio/demo_output.wav"
     data-label-a="In" data-label-b="Out — through encoder and decoder"></div>

16 numbers are a handle on the sound: change them, and the decoder makes something new.

Note:
    - Rendered with scripts/render_demo.py, block by block, 2048 samples at a time — exactly what we do in C++. The input is synthesized there.
    - The fader crossfades what you hear; the spectrogram you hear is the brighter one.

---

## Neural network integration

{{INTEGRATION:implement}}

The first two are done. **Today starts at the third.**

Note:
    - From the ADC 2024 talk (Real-Time Inference of Neural Networks, Part II).
