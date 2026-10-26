<h1>Step 11<br>Hands on the latents</h1>

Two models, one pool

<!-- .slide: data-state="no-header" -->
<!-- kind: demo, exercise -->

Note:
    - Demo first: the webcam, the hand, the piano. Then one TODO, the mapping.

---

<div class="tag">Two models</div>

## A camera at 30 fps,<br><span class="then">audio at 44.1 kHz, one pool.</span>

<div class="cards">
  <div class="card">{{ICON:people}}<div class="card-title">camera</div><div class="card-text">30 frames per second</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:model}}<div class="card-title">hand tracking</div><div class="card-text">palm → landmarks → gesture<br>three models, ONNX Runtime</div></div>
  <div class="card-arrow">→</div>
  <div class="card accent">{{ICON:sliders}}<div class="card-title">8 latents</div><div class="card-text">height, pinch, tilt, …</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:model}}<div class="card-title">decoder</div><div class="card-text">a generator, no audio in</div></div>
</div>

Note:
    - Two kinds of model in one pool: the tracker's three run once per camera frame, the decoder as a hard stream. The pool does not care which is which.
    - The tracker is anira-cam's, on anira's 2.x C++ API; the v3 library still carries it. The decoder is step 10's, on the v3 handler, as a generator.
    - The decoder here has no audio input at all: the audio thread hands it one frame of the current latents per 2048 samples it pulls, two frames ahead. The latents come from the hand, smoothed with a one-pole.
    - Without a hand, the latents glide to rest and the output fades.

---

<div class="tag">Given</div>

## The demo, with one gap.<br><span class="then">Camera on macOS; an XY pad everywhere else.</span>

<div class="given">
  <div class="given-column">
    <div class="given-label">Models</div>
    <div class="given-folder">models/</div>
    {{FILE:model_decoder.onnx}}
    {{FILE:hand_detector.onnx|new}}
    {{FILE:hand_landmarks_detector.onnx|new}}
  </div>
  <div class="given-column many">
    <div class="given-label">Helpers</div>
    <div class="given-folder">11_hands/ · source/</div>
    {{FILE:HandTracker.h|new}}
    {{FILE:LatentDecoder.cpp|new}}
    {{FILE:CameraInput.mm|new}}
    {{FILE:decoder.json}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">11_hands/exercise/</div>
    {{FILE:HandControls.cpp}}
  </div>
</div>

Note:
    - The camera is platform code: AVFoundation on macOS, juce::CameraDevice on Windows. Linux has no camera in JUCE: the video area is a pad, the mouse is the lead hand, same features, same mapping, same TODO.
    - Build: -DWORKSHOP_JUCE=ON, target step11_exercise; JUCE is cloned once.
    - If time is short this step is the one to cut: run the demo, skip the TODO.

---

<div class="tag">Task: <code>HandControls.cpp</code></div>

## Task

<div class="task-timer" data-minutes="10"></div>

Fill in the TODO in `steps/11_hands/exercise/HandControls.cpp`

```bash
cmake --build --preset release --target step11_exercise
./build/bin/step11_exercise
```

<div class="run">
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>1</span>Hand → latents</div>
    <div class="run-visual run-setup">
      <div class="run-note">open → z0 · height → z2 · the rest by ear</div>
      <div class="run-ok">✓ the piano follows the hand</div>
    </div>
  </div>
</div>

---

<div class="tag">Solution</div>

## TODO 1 · Hand → latents

```cpp
// role, feature, latent at feature 0, at feature 1, at rest
constexpr std::array<LatentMapping, k_num_latents> k_mappings{{
    {Role::Lead,    Feature::Openness, -2.5F,  2.0F, -3.0F, "dynamics"},
    {Role::Lead,    Feature::Pinch,     0.5F, -3.0F,  0.0F, "body"},
    {Role::Lead,    Feature::Y,        -2.5F,  3.0F,  0.0F, "register"},
    {Role::Lead,    Feature::X,        -2.5F,  2.5F,  0.0F, "colour"},
    {Role::Texture, Feature::Y,        -2.5F,  3.0F,  0.0F, "swell"},
    {Role::Lead,    Feature::Roll,     -3.0F,  3.0F,  0.0F, "tilt"},
    {Role::Texture, Feature::X,        -3.0F,  3.0F,  0.0F, "dark/bright"},
    {Role::Texture, Feature::Openness,  0.0F,  3.0F,  0.0F, "resonance"},
}};
```

Note:
    - The ranges come from the latent analysis of the model: about ±2 is the natural range per dimension, z0 is monotonic in loudness, z2 correlates with pitch at 0.96. Rest for z0 is −3: close to silent.
    - Everything here is main-thread code: the hand arrives at 30 Hz, the audio thread only reads the smoothed atomics.

---

<div class="tag">Done</div>

## Two models, two rates,<br><span class="then">one library.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
    - Every frame still goes camera → CPU → engine as a copy. That is the last slide.
