<h1>Step 10<br>Encoder, latents, decoder</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
    - The model from the slides, cut in the middle: eight numbers per block, and a hand on them.

---

<div class="tag">Motivation</div>

## 2048 samples in, 8 numbers between.<br><span class="then">The middle is where you can reach in.</span>

<div class="cards">
  <div class="card">{{ICON:model}}<div class="card-title">encoder</div><div class="card-text">audio → z<br><code>model_encoder.onnx</code></div></div>
  <div class="card-arrow">→</div>
  <div class="card accent">{{ICON:sliders}}<div class="card-title">z · 8 latents</div><div class="card-text">bias and scale, per dimension</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:model}}<div class="card-title">decoder</div><div class="card-text">z → audio<br><code>model_decoder.onnx</code></div></div>
</div>

Note:
    - The VAE figure from the first chapter, now as two files. The encoder emits one latent frame per 2048 samples; the decoder turns one frame back into 2048 samples.
    - What the dimensions do (from the latent analysis): z0 loudness, z2 register, z4 swell. Moving them is the instrument.

---

<div class="tag">Goal</div>

## Two handlers, one context.<br><span class="then">Your code between them.</span>

<div class="statement-points">
  <div><small>1</small>The encoder: audio in, a latent frame out, at 1/2048 of the rate</div>
  <div><small>2</small>Bias and scale on z, on the audio thread</div>
  <div><small>3</small>The decoder: a latent frame in, audio out; the latency is the sum, frames × 2048 + samples</div>
</div>

---

<div class="tag">Given</div>

## 2 models, 4 config files, 1 file to edit.

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">models/</div>
    {{FILE:model_encoder.onnx|new}}
    {{FILE:model_decoder.onnx|new}}
  </div>
  <div class="given-column many">
    <div class="given-label">Helpers</div>
    <div class="given-folder">10_split/</div>
    {{FILE:encoder.json|new}}
    {{FILE:decoder.json|new}}
    {{FILE:contract.json}}
    {{FILE:anira}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">10_split/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
    - Both model files carry their own state pair, as in step 9: the encoder's 34,304 floats, the decoder's 29,184.
    - Nobody buffers between the two handlers but anira: a frame popped from the encoder is pushed into the decoder in the same callback; the decoder's latent ring holds it until its next block, and its latency accounts for it.
    - The decoder anchors on its audio output: its clock is the audio it produces, and the latent input follows at one frame per 2048 samples.

---

<div class="tag">Helpers: <code>decoder.json</code></div>

## A generator: the clock is the output.

```json
{
  "models":  [ { "engine": "onnxruntime", "path": "../../models/model_decoder.onnx" } ],
  "anchor":  "audio",
  "inputs":  [ { "name": "latents", "role": "streamed",
                 "axes": [["batch",1],["channel",8],["time",1]],
                 "window": { "min": 1, "max": 1 } },
               { "name": "state_in", "role": "state", … } ],
  "outputs": [ { "name": "audio", "role": "streamed", "latency": 528,
                 "axes": [["batch",1],["channel",1],["time",2048]],
                 "window": { "min": 2048, "max": 2048 } },
               … ]
}
```

Note:
    - "anchor": the streamed tensor the host geometry refers to. For the decoder that is the audio output; the latent input's rate follows from the two windows, 1 per 2048.
    - The model's own delay is split: 1696 samples in the encoder's network, 528 in the decoder's filter bank. Each file declares its part.

---

<div class="tag">Task: <code>main.cpp</code></div>

## Task

<div class="task-timer" data-minutes="20"></div>

Fill in the TODOs in `steps/10_split/exercise/main.cpp`

```bash
cmake --build --preset release --target step10_exercise
./build/bin/step10_exercise
```

<div class="run">
  <div class="run-step" data-seconds="2.6">
    <div class="run-title"><span>1</span>The encoder</div>
    <div class="run-visual run-worker">
      <div class="q small one"><div class="q-slot"><b></b></div></div>
      <div class="mini-model">encoder</div>
      <div class="run-note">8 × 1 per 2048</div>
    </div>
  </div>
  <div class="run-step" data-seconds="2.6">
    <div class="run-title"><span>2</span>Bias and scale on z</div>
    <div class="run-visual run-setup">
      <div class="run-note"><code>z[i] = z[i] * scale[i] + bias[i]</code></div>
    </div>
  </div>
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>3</span>The decoder</div>
    <div class="run-visual run-worker">
      <div class="mini-model">decoder</div>
      <div class="q small one out"><div class="q-slot"><b></b></div></div>
      <div class="run-ok">✓ latency = encoder + decoder</div>
    </div>
  </div>
</div>

---

<div class="tag">Solution</div>

## TODO 1 · The encoder

```cpp
anira::Handler encoder(context, anira::Pipeline{anira::stage::Inference(encoder_model)});
encoder.prepare(contract);                       // geometry: the host block, at 44.1 kHz

// Audio in, latent frames out: one frame of 8 per 2048 samples, when a block is complete
encoder.push_data(block, 0);
while (encoder.pop_data(frame, 0, delivered), delivered == 1) { … }   // one frame at a time
```

Note:
    - push and pop instead of process: the two sides run at different rates, so the output is collected separately. A frame comes back about one latency later; delivered says whether one came back in this callback.
    - What comes out goes straight into the decoder (TODO 3), in the same callback. No buffer of ours in between.

---

<div class="tag">Solution</div>

## TODO 2 · Bias and scale on z

```cpp
for (size_t i = 0; i < 8; ++i) {
    frame[i] = frame[i] * m_scale[i].load() + m_bias[i].load();
}
```

Note:
    - Atomics, so the sliders may move while the audio thread reads. Nothing else: no allocation, no lock.
    - All zero bias and unit scale: the output is the forward model's, to 1e-6. That is the check.

---

<div class="tag">Solution</div>

## TODO 3 · The decoder

```cpp
anira::Handler decoder(context, anira::Pipeline{anira::stage::Inference(decoder_model)});
decoder.prepare(contract);                       // anchored on its audio output

decoder.push_data(frame, 0);                     // one latent frame
decoder.pop_data(block, 0);                      // 512 samples of audio, or the miss policy

const size_t latency = encoder.get_latency() * 2048   // the encoder counts frames
                     + decoder.get_latency();         // the decoder counts samples
```

Note:
    - The encoder's latency is in frames, so times 2048. The decoder's is in samples. The sum is what the host is told.
    - Both handlers share the context: one pool, two stateful streams, each in order.

---

<div class="tag">Done</div>

## The model, taken apart.<br><span class="then">Eight numbers in your hand.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
    - Bias on z0 and z2, played live: louder, higher. Then the hand that moves them.
