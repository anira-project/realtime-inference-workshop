<h1>Step 11<br>Hands on the latents</h1>

Two models, one pool

<!-- .slide: data-state="no-header" -->
<!-- kind: demo, exercise -->

Note:
- Step 11: hands on the latents, two models in one pool.
- This is the decoder from Step 10, and a hand moves its eight numbers.
- We play it now; the exercise needs a JUCE build, so its slides are for at home.

---

<div class="tag">Two models</div>

## A camera at 30 fps,<br><span class="then">audio at 44.1 kHz, one pool.</span>

<div class="cards">
  <div class="card">{{ICON:people}}<div class="card-title">camera</div><div class="card-text">30 frames per second</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:model}}<div class="card-title">hand tracking</div><div class="card-text">palm → landmarks → gesture<br>four models, ONNX Runtime</div></div>
  <div class="card-arrow">→</div>
  <div class="card accent">{{ICON:sliders}}<div class="card-title">8 latents</div><div class="card-text">height, pinch, tilt, …</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:model}}<div class="card-title">decoder</div><div class="card-text">a generator, no audio in</div></div>
</div>

Note:
- Two kinds of model share one pool: one runs per camera frame, the other per audio block.
- The hand tracker is MediaPipe's chain of four small ONNX models, run once per camera frame, 30 times a second.
- The decoder runs as a hard real-time audio stream, as a generator, with no audio input at all.
- The hand becomes eight latents, smoothed on the main thread, and the audio thread reads them as atomics.
- [Start the demo. Speak little, play more.]
- When I open my hand, it gets louder; that is z0.
- When I raise it, the register goes up; that is z2.
- Pinch, left and right, and tilt change body, colour and tilt.
- The second hand sets the texture: swell, dark and bright, resonance.
- When I take my hands away, the latents glide to rest and the sound fades.

---

<div class="tag">Given</div>

## The demo, with one gap.<br><span class="then">Camera on macOS and Windows; an XY pad on Linux.</span>

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
- The mapping you just heard is the one TODO, in HandControls.cpp.
- It is main-thread code: the hand arrives 30 times a second, and the audio thread only reads.
- On Linux there is an XY pad instead of the camera, with the same TODO.
- The app needs the JUCE build, which takes a few minutes, so try it at home.

---

<div class="tag">Task: <code>HandControls.cpp</code></div>

## Task

<div class="task-timer" data-minutes="10"></div>

Fill in the TODO in `steps/11_hands/exercise/HandControls.cpp`

```bash
cmake --preset release -DWORKSHOP_JUCE=ON      # once, with JUCE
cmake --build --preset release --target step11_exercise
open build/steps/11_hands/step11_exercise_artefacts/Release/*.app
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

Note:
- This is the task for at home, sized at ten minutes.
- Configure with WORKSHOP_JUCE on, build step11_exercise, and open the app; the README has the commands.
- Openness goes to z0, height to z2, and the rest you set by ear.
- It works when the piano follows the hand.

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
- Each row takes one feature of one hand to one latent.
- A feature runs from 0 to 1, for example fist to open hand.
- The row gives the latent's value at 0, at 1, and when no hand is there.
- About plus or minus 2 is the natural range of each latent in this model.
- z0 follows loudness, and z2 follows pitch closely.
- z0 rests at minus 3, which is close to silence, so no hand means no sound.

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
- Two models, two rates, one library.
- One thing is still a copy: every camera frame goes to the CPU and then to the engine.
- That is where the outlook starts.
