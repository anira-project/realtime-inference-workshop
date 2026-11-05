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
  <div class="card accent">{{ICON:sliders}}<div class="card-title">z · 8 latents</div><div class="card-text">an offset per dimension</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:model}}<div class="card-title">decoder</div><div class="card-text">z → audio<br><code>model_decoder.onnx</code></div></div>
</div>

Note:
- This is the model from the introduction, a variational autoencoder, now as two files.
- The encoder turns 2048 samples into one latent frame of eight numbers; we call them z.
- The decoder turns that frame back into 2048 samples.
- In the middle you can reach in, with an offset on each of the eight numbers, as in anira's own example.
- Two of them you will hear in a moment: z0 is loudness, z2 is register.

---

<div class="tag">Latent space</div>

## An autoencoder squeezes audio through 8 numbers.<br><span class="then">A variational one leaves no gaps between them.</span>

<svg class="lat-fig" viewBox="0 0 1240 420" aria-label="latent codes of an autoencoder and of a variational autoencoder">
  <g transform="translate(40 0)">
    <rect class="frame" x="0" y="20" width="540" height="330" rx="18"/>
    <circle class="pt" cx="110" cy="110" r="9"/><circle class="pt" cx="135" cy="95" r="9"/><circle class="pt" cx="125" cy="135" r="9"/>
    <circle class="pt" cx="420" cy="90" r="9"/><circle class="pt" cx="445" cy="115" r="9"/>
    <circle class="pt" cx="170" cy="280" r="9"/><circle class="pt" cx="200" cy="300" r="9"/><circle class="pt" cx="150" cy="305" r="9"/>
    <circle class="pt" cx="430" cy="290" r="9"/><circle class="pt" cx="405" cy="265" r="9"/>
    <text class="gap" x="290" y="205">?</text>
    <text class="lab" x="270" y="395">autoencoder: points, gaps between</text>
  </g>
  <g transform="translate(660 0)">
    <rect class="frame" x="0" y="20" width="540" height="330" rx="18"/>
    <circle class="prior" cx="270" cy="185" r="150"/>
    <circle class="blob" cx="210" cy="140" r="62"/><circle class="blob" cx="320" cy="130" r="58"/>
    <circle class="blob" cx="230" cy="235" r="60"/><circle class="blob" cx="330" cy="230" r="64"/>
    <circle class="blob" cx="270" cy="185" r="52"/>
    <circle class="pt" cx="210" cy="140" r="7"/><circle class="pt" cx="320" cy="130" r="7"/>
    <circle class="pt" cx="230" cy="235" r="7"/><circle class="pt" cx="330" cy="230" r="7"/><circle class="pt" cx="270" cy="185" r="7"/>
    <text class="lab muted small" x="445" y="62">N(0, 1)</text>
    <text class="lab" x="270" y="395">variational: a mean and a spread, one space</text>
  </g>
</svg>

Note:
- An autoencoder has two halves: the encoder squeezes 2048 samples into a few numbers, the decoder rebuilds the audio from them. Those few numbers are the latent space; one point in it is z.
- A plain autoencoder only learns to rebuild what it saw: each sound is a point, and between the points nothing is promised. Move z into a gap and the decoder may give you anything.
- A variational autoencoder predicts a mean and a spread for every frame, and trains on samples drawn around the mean: the decoder learns whole regions, not points.
- A second term in the loss, the KL divergence, pulls all of them towards one standard normal: the regions overlap and fill one space.
- That is why we can reach in: a small change in z is a small, plausible change in the sound.
- Sources: Kingma and Welling 2013, arXiv 1312.6114; Lilian Weng, "From Autoencoder to Beta-VAE".

---

<div class="tag">Latent space</div>

## 128 dimensions trained, 8 kept.<br><span class="then">Sorted by how much they vary.</span>

<svg class="lat-fig" viewBox="0 0 1240 420" aria-label="variance per latent dimension">
  <rect class="kept" x="60" y="72" width="56" height="258" rx="4"/>
  <text class="dim" x="88" y="360">z0</text>
  <rect class="kept" x="130" y="120" width="56" height="210" rx="4"/>
  <text class="dim" x="158" y="360">z1</text>
  <rect class="kept" x="200" y="126" width="56" height="204" rx="4"/>
  <text class="dim" x="228" y="360">z2</text>
  <rect class="kept" x="270" y="180" width="56" height="150" rx="4"/>
  <text class="dim" x="298" y="360">z3</text>
  <rect class="kept" x="340" y="182" width="56" height="148" rx="4"/>
  <text class="dim" x="368" y="360">z4</text>
  <rect class="kept" x="410" y="201" width="56" height="129" rx="4"/>
  <text class="dim" x="438" y="360">z5</text>
  <rect class="kept" x="480" y="222" width="56" height="108" rx="4"/>
  <text class="dim" x="508" y="360">z6</text>
  <rect class="kept" x="550" y="238" width="56" height="92" rx="4"/>
  <text class="dim" x="578" y="360">z7</text>
  <rect class="dropped" x="620" y="267" width="56" height="63" rx="4"/>
  <rect class="dropped" x="690" y="295" width="56" height="35" rx="4"/>
  <rect class="dropped" x="760" y="327" width="56" height="3" rx="4"/>
  <rect class="dropped" x="830" y="328" width="56" height="2" rx="4"/>
  <rect class="dropped" x="900" y="328" width="56" height="2" rx="4"/>
  <rect class="dropped" x="970" y="328" width="56" height="2" rx="4"/>
  <rect class="dropped" x="1040" y="328" width="56" height="2" rx="4"/>
  <rect class="dropped" x="1110" y="328" width="56" height="2" rx="4"/>
  <line class="axis" x1="40" y1="330" x2="1186" y2="330"/>
  <text class="lab" x="333" y="400">8 kept: 92.8 % of the variance</text>
  <text class="lab muted" x="893" y="400">120 dropped: zeros in our decoder</text>
  <text class="lab muted small" x="1126" y="310">… 128</text>
</svg>

Note:
- This is the Realtime Audio Variational autoEncoder, Caillon and Esling 2021, arXiv 2111.05011.
- 2048 samples per frame: a 16-band PQMF filter bank first, then encoder strides of 4, 4, 4 and 2. 16 × 128 = 2048.
- Training has two stages: first encoder and decoder as a variational autoencoder, then the encoder frozen and the decoder fine-tuned adversarially, for the sound quality.
- After training, a PCA of the encoder's means sorts the 128 dimensions by how much they vary over the training data. The export keeps the first ones.
- Our model keeps 8. The bars are from the model file: z0 alone is 18.4 % of the variance, all 8 together 92.8 %.
- The other 120 are dropped. The original fills them with noise; our export fills them with zeros, so the decoder is deterministic and the check can compare.
- The encoder gives the mean, not a sample: the same audio gives the same z.

---

<div class="tag">Latent space</div>

## Working with z.<br><span class="then">Everything between encode and decode.</span>

<div class="cards">
  <div class="card accent"><div class="card-title">offset</div><div class="card-text"><code>z + b</code><br>this step</div></div>
  <div class="card"><div class="card-title">scale</div><div class="card-text"><code>z · s</code></div></div>
  <div class="card"><div class="card-title">noise</div><div class="card-text"><code>z + ε</code></div></div>
  <div class="card"><div class="card-title">blend</div><div class="card-text"><code>(1−a)·z<sub>A</sub> + a·z<sub>B</sub></code></div></div>
  <div class="card"><div class="card-title">drive</div><div class="card-text">LFO, MIDI, a hand<br>Step 11</div></div>
</div>

<div class="statement-points">
  <div>Once per latent frame, 21.5 times a second, on the audio thread</div>
  <div>What a dimension means is learned: in our model z0 follows loudness, z2 the register</div>
</div>

Note:
- Everything happens between encode and decode, once per latent frame, 21.5 times a second.
- An offset, a bias, shifts a dimension: that is this step, eight knobs, as in anira's example.
- A scale widens or narrows how far a dimension moves; scale zero plus an offset hands the dimension over to the knob completely.
- Noise on z adds variation; blending the z of two sounds crossfades in the latent space, not in the audio.
- Or drive z from outside, with no encoder at all: an LFO, MIDI, or in Step 11 a hand in front of a camera.
- The same operations are in IRCAM's tools for this model: nn~ for Max and Pure Data exposes encode and decode, and its plugin has bias, scale and noise per dimension.
- What a dimension means is learned, not designed, and differs from model to model. z0 carries the most variance; for ours, z0 follows loudness and z2 the register. Listen before you name one.
- Sources: github.com/acids-ircam/nn_tilde; forum.ircam.fr, "Neural synthesis in a DAW with RAVE".

---

<div class="tag">Goal</div>

## Two handlers, one context.<br><span class="then">Your code between them.</span>

<div class="statement-points">
  <div><small>1</small>The latency the host is told: frames × 2048 + samples</div>
  <div><small>2</small>The encoder: audio in, a latent frame out, at 1/2048 of the rate</div>
  <div><small>3</small>An offset on z, on the audio thread</div>
  <div><small>4</small>The decoder: a latent frame in, audio out</div>
</div>

Note:
- We use two handlers in one context, which means one pool, and your code sits between them.
- First, the latency the host is told: the encoder counts in frames, the decoder in samples.
- Second, the encoder: audio in, a frame out, at 1/2048 of the rate.
- Third, an offset on z, on the audio thread: eight knobs, one per latent.
- Fourth, the decoder: a frame in, audio out.
- The structure is the one of anira's own encoder/decoder example.

---

<div class="tag">Rates</div>

## Two rates in one chain.<br><span class="then">Samples for the audio, frames for the latents.</span>

<div class="lat-sum">
  <div class="lat-part">audio<small>samples a second</small><strong>44100</strong></div>
  <div class="lat-plus">÷</div>
  <div class="lat-part">one latent frame<small>samples</small><strong>2048</strong></div>
  <div class="lat-plus">=</div>
  <div class="lat-part total">latent frames<small>a second</small><strong>≈ 21.5</strong></div>
</div>

<div class="statement-points">
  <div>A frame is due every 2048 input samples: every fourth 512-sample callback</div>
  <div>The encoder's latency comes in frames: × 2048 for samples</div>
  <div>anira counts both handlers in audio samples: their anchors are the audio</div>
</div>

Note:
- Before the task: two rates in one chain.
- The audio runs at 44,100 samples a second. The latents come one frame per 2048 samples, about 21.5 frames a second.
- So in the callback a latent frame is due every 2048 input samples, every fourth callback of 512.
- The encoder reports its latency in frames, its output's unit. For the host we need samples: frames times 2048.
- anira itself sees only audio samples: the encoder's anchor is its audio input, the decoder's its audio output. The frame rate lives in our code.

---

<div class="tag">Given</div>

## 2 models, 3 config files, 1 file to edit.

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">common/assets/models/</div>
    {{FILE:model_encoder.onnx|new}}
    {{FILE:model_decoder.onnx|new}}
    {{FILE:target_split_signal.h|new}}
  </div>
  <div class="given-column many">
    <div class="given-label">Helpers</div>
    <div class="given-folder">10_split/ · common/</div>
    {{FILE:encoder.json|new}}
    {{FILE:decoder.json|new}}
    {{FILE:contract.json}}
    {{FILE:PluginProcessor.cpp}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">10_split/exercise/</div>
    {{FILE:SplitProcessor.cpp|new}}
  </div>
</div>

Note:
- Both models carry their own state pair, as in Step 9.
- Their configs, encoder.json and decoder.json, are given, and so is the contract: like Step 9's, but on_miss zeros, because bypass needs an input of the same shape as the output.
- Between the two handlers you write no buffer: anira's queues are the only ones.
- The plugin around it is given: Step 9's, with eight knobs, z0 to z7.
- Step 10 has a reference of its own, target_split_signal.h: encoder and decoder as anira's example runs them.
- You edit only SplitProcessor.cpp.

---

<!-- .slide: class="dense-code" -->

<div class="tag">Helpers: <code>decoder.json</code></div>

## A generator: the clock is the output.

```json
{
  "models":  [ { "engine": "onnxruntime",
                 "path": "../../common/assets/models/rave/model_decoder.onnx" } ],
  "anchor":  "audio",
  "inputs":  [ { "name": "latents", "role": "streamed",
                 "axes": [["batch",1],["channel",8],["time",1]],
                 "window": { "min": 1, "max": 1 } },
               { "name": "state_in", "role": "state",
                 "axes": [["batch",1],["any",29184]], "state_source": "state_out" } ],
  "outputs": [ { "name": "audio", "role": "streamed", "latency": 2224,
                 "axes": [["batch",1],["channel",1],["time",2048]],
                 "window": { "min": 2048, "max": 2048 } },
               { "name": "state_out", "role": "state", "axes": [["batch",1],["any",29184]] } ]
}
```

Note:
- The decoder has no audio input: it generates audio from latent frames. This is the whole decoder.json, only the dtype is left out: float32 is the default.
- So its anchor, the tensor whose time the host's blocks count in, is the audio output.
- The latent input follows at one frame per 2048 samples.
- The model's own delay, 2224, is declared once, where the audio comes out.

---

<div class="tag">Task: <code>SplitProcessor.cpp</code></div>

## Task

<div class="task-timer" data-minutes="10"></div>

Fill in the TODOs in `steps/10_split/exercise/SplitProcessor.cpp`

```bash
# validate: the check against the reference
cmake --build --preset release --target step10_exercise_check
./build/bin/step10_exercise_check
# play: the same processor as a plugin, eight knobs (or step10_exercise_VST3)
cmake --build --preset release --target step10_exercise_Standalone
```

<div class="run">
  <div class="run-step" data-seconds="2">
    <div class="run-title"><span>1</span>The latency</div>
    <div class="run-visual run-setup">
      <div class="run-note">frames × 2048 + samples</div>
    </div>
  </div>
  <div class="run-step" data-seconds="2.6">
    <div class="run-title"><span>2</span>The encoder</div>
    <div class="run-visual run-worker">
      <div class="q small one"><div class="q-slot"><b></b></div></div>
      <div class="mini-model">encoder</div>
      <div class="run-note">one frame of 8 per 2048 samples</div>
    </div>
  </div>
  <div class="run-step" data-seconds="2.4">
    <div class="run-title"><span>3</span>An offset on z</div>
    <div class="run-visual run-setup">
      <div class="run-note"><code>z[i] += ?</code></div>
    </div>
  </div>
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>4</span>The decoder</div>
    <div class="run-visual run-worker">
      <div class="mini-model">decoder</div>
      <div class="q small one out"><div class="q-slot"><b></b></div></div>
      <div class="run-ok">✓ matches the reference, z moves it</div>
    </div>
  </div>
</div>

Note:
- You have ten minutes.
- Fill in the four TODOs in SplitProcessor.cpp, then build step10_exercise_check and run it.
- Once it passes, build the plugin: the same SplitProcessor, with eight knobs, z0 to z7.
- With all offsets at zero, the output has to match step 10's reference: encoder and decoder as anira's example runs them.
- The check also needs the right latency, encoder and decoder together, and an offset on z0 that changes the output.

---

<div class="tag">Solution</div>

## TODO 1 · The latency

```cpp
m_encoder->prepare(m_contract);
m_decoder->prepare(m_contract);
m_encoder_latency_frames = m_encoder->get_latency(0);       // In latent frames
const size_t decoder_latency = m_decoder->get_latency(0);   // In samples

m_latency = m_encoder_latency_frames * k_frame_size + decoder_latency;   // 1 × 2048 + 6320 = 8368
```

Note:
- TODO 1 is the latency the host is told.
- Each handler reports its own, in the unit of its output: the encoder in latent frames, the decoder in samples.
- The encoder reports one frame: 2048 samples. The decoder reports 6320, the same as the forward model in Step 9: 2224 of model plus the block handling, for host blocks from 1 to 512.
- The sum is 8368, one encoder frame more than the forward model.
- As in anira's example, nothing is declared in the contract: the encoder's priming frame keeps the decoder fed from the start.

---

<div class="tag">Solution</div>

## TODO 2 · The encoder

```cpp
m_input_samples += num_samples;
m_encoder->push_data(block, 0);
while (m_input_samples >= k_frame_size) {
    const anira_status popped = m_encoder->pop_data(m_frame, 0);
    if (popped != ANIRA_OK && popped != ANIRA_MISSED) { break; }  // A missed frame keeps its place
    m_input_samples -= k_frame_size;
    ++m_frames;
    …
}
```

Note:
- TODO 2 is the encoder.
- We use push and pop instead of process, because audio goes in at one rate and frames come out at another.
- We count the input samples and pop exactly one frame per 2048, about 21.5 a second, as anira's own encoder/decoder example does.
- A frame that is not there yet is a miss: the miss policy fills it, it keeps its place in the stream, and anira drops the late one when it arrives.
- The first frame is the encoder's priming, its latency: a frame of zeros. It goes on to the decoder like every other frame.
- That zero frame is not silence, it changes the decoder's state: that is why step 10 has a reference of its own.

---

<div class="tag">Solution</div>

## TODO 3 · An offset on z

```cpp
for (size_t i = 0; i < k_num_latents; ++i) {
    m_frame_data[i] += m_offset[i].load();
}
```

Note:
- TODO 3 is an offset on z: one loop over eight numbers, on the audio thread, as in anira's example.
- The offsets are atomics: the plugin sets them from its eight knobs, z0 to z7, at the start of every callback, with no lock and no allocation.
- The check sets z0 to 1 and expects the output to change.

---

<div class="tag">Solution</div>

## TODO 4 · The decoder

```cpp
while (m_input_samples >= k_frame_size) {
    …
    m_decoder->push_data(m_frame, 0);   // Every frame
}
const anira_status status = m_decoder->pop_data(block, 0);   // This callback's audio
return status == ANIRA_MISSED;   // The miss policy filled the block
```

Note:
- TODO 4 is the decoder.
- Every frame goes in, inside the loop; once per callback, the decoder's audio comes out into the block.
- The decoder's anchor is its audio output: it gives exactly the callback's samples, or the miss policy fills them.

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
- [Play it: the offset on z0 and z2 live, louder, then higher.]
- That is the model taken apart, with eight numbers in your hand.
- Next: a hand that moves them.
