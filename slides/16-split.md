<h1>Step 10<br>Encoder, latents, decoder</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- Step 10: encoder, latents, decoder.
- Now we cut the model in the middle.

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
- This is the variational autoencoder from the introduction, now as two files.
- The encoder turns 2048 samples into one latent frame of eight numbers; we call them z.
- The decoder turns that frame back into 2048 samples.
- In the middle you can reach in, with a bias and a scale on each of the eight numbers.
- Two of them you will hear in a moment: z0 is loudness, z2 is register.

---

<div class="tag">Goal</div>

## Two handlers, one context.<br><span class="then">Your code between them.</span>

<div class="statement-points">
  <div><small>1</small>The encoder: audio in, a latent frame out, at 1/2048 of the rate</div>
  <div><small>2</small>Bias and scale on z, on the audio thread</div>
  <div><small>3</small>The decoder: a latent frame in, audio out; it declares the sum, frames × 2048 + samples</div>
</div>

Note:
- We use two handlers in one context, which means one pool, and your code sits between them.
- First, the encoder: audio in, a frame out, at 1/2048 of the rate.
- Second, bias and scale on z, on the audio thread.
- Third, the decoder: a frame in, audio out, and it declares the total latency.

---

<div class="tag">Given</div>

## 2 models, 3 config files, 1 file to edit.

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">models/</div>
    {{FILE:model_encoder.onnx|new}}
    {{FILE:model_decoder.onnx|new}}
  </div>
  <div class="given-column many">
    <div class="given-label">Helpers</div>
    <div class="given-folder">10_split/exercise/ · common/helpers/</div>
    {{FILE:encoder.json|new}}
    {{FILE:decoder.json|new}}
    {{FILE:contract.json}}
    {{FILE:support.h}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">10_split/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
- Both models carry their own state pair, as in Step 9.
- Their configs, encoder.json and decoder.json, are given, and the contract is the one from Step 9.
- Between the two handlers you write no buffer: anira's queues are the only ones.
- You edit only main.cpp.

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
  "outputs": [ { "name": "audio", "role": "streamed", "latency": 2224,
                 "axes": [["batch",1],["channel",1],["time",2048]],
                 "window": { "min": 2048, "max": 2048 } },
               … ]
}
```

Note:
- The decoder has no audio input: it generates audio from latent frames.
- So its anchor, the tensor whose time the host's blocks count in, is the audio output.
- The latent input follows at one frame per 2048 samples.
- The model's own delay, 2224, is declared once, where the audio comes out.

---

<div class="tag">Task: <code>main.cpp</code></div>

## Task

<div class="task-timer" data-minutes="10"></div>

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
      <div class="run-note">8 × 1 per 2048 · skip the priming</div>
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
      <div class="run-ok">✓ declared: encoder + decoder</div>
    </div>
  </div>
</div>

Note:
- You have ten minutes.
- Fill in the TODOs in main.cpp, then build step10_exercise and run it.
- With zero bias and unit scale, the output has to match the forward model from Step 9.
- The check also needs the right latency: encoder and decoder together.

---

<div class="tag">Solution</div>

## TODO 1 · The encoder

```cpp
create(encoder, context, step_dir + "/encoder.json");
encoder_latency_frames = prepare(encoder, contract);   // in latent frames

// The callback: audio in, latent frames out, one per 2048 samples
anira_handler_push_data(encoder.m_handle, &block, 0);
while (anira_handler_get_available_samples(encoder.m_handle, 0, 0, &available) == ANIRA_OK
       && available > 0) {
    anira_handler_pop_data(encoder.m_handle, &frame, 0, nullptr);
    if (frames++ < encoder_latency_frames) { continue; }   // priming: not silence
    …
}
```

Note:
- TODO 1 is the encoder.
- We use push and pop instead of process, because audio goes in at one rate and frames come out at another.
- get_available_samples tells us whether a frame is waiting, and we pop only then.
- A pop with nothing waiting counts as a miss.
- The first frame is the encoder's priming, its latency: a frame of zeros.
- A zero latent frame is not silence, the decoder turns it into sound, so we skip it.

---

<div class="tag">Solution</div>

## TODO 2 · Bias and scale on z

```cpp
for (size_t i = 0; i < 8; ++i) {
    frame[i] = frame[i] * m_scale[i].load() + m_bias[i].load();
}
```

Note:
- TODO 2 is bias and scale on z: one loop over eight numbers, on the audio thread.
- Scale and bias are atomics, so a slider on the main thread can move them while audio runs, with no lock and no allocation.

---

<div class="tag">Solution</div>

## TODO 3 · The decoder

```cpp
create(decoder, context, step_dir + "/decoder.json");
const size_t decoder_latency = prepare(decoder, contract);   // in samples, on its own
latency = encoder_latency_frames * 2048 + decoder_latency;   // 2048 + 4272
contract.hard_latency("audio", latency);                     // the frame arrives late
prepare(decoder, contract);                                  // reports 6320 now

// The callback: every real frame in, then the audio for this callback out
anira_handler_push_data(decoder.m_handle, &frame, 0);
anira_handler_pop_data(decoder.m_handle, &block, 0, nullptr);   // or the miss policy
```

Note:
- TODO 3 is the decoder.
- Prepared on its own, it reports 4272, the same as the forward model in Step 9: 2224 of model plus the block handling.
- That assumes a frame is there when its window starts, but ours arrives one encoder frame late.
- So we declare the sum in the contract, 2048 plus 4272, and prepare again.
- That gives 6320, the number the host is told, and the same number we built by hand in Step 6.

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
- [Play it: bias on z0 and z2 live, louder, then higher.]
- That is the model taken apart, with eight numbers in your hand.
- Next: a hand that moves them.
