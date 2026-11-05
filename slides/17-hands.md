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

<div class="tag">Hand tracking</div>

## MediaPipe hands<br><span class="then">Three stages, four models.</span>

<div class="cards">
  <div class="card">{{ICON:people}}<div class="card-title">camera frame</div><div class="card-text">30 a second</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:model}}<div class="card-title">palm detector</div><div class="card-text">192 × 192 image<br>→ a box per palm</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:model}}<div class="card-title">landmarks</div><div class="card-text">224 × 224 crop per hand<br>→ 21 joints, left or right</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:model}}<div class="card-title">gesture</div><div class="card-text">embedder + classifier<br>→ 7 gestures</div></div>
</div>

Note:
- The hand tracking is Google's MediaPipe hand chain: four small models, Apache 2.0, converted to ONNX in anira-cam.
- The palm detector looks at the whole frame, scaled to 192 by 192, and finds boxes around palms.
- It only runs while fewer hands are tracked than we want, here two.
- The landmark model gets a 224 by 224 crop around one hand and returns 21 joints, whether a hand is there, and whether it is a left or a right hand.
- Once a hand is found, the next frame's crop comes from its own joints, so the detector can rest.
- The gesture stage turns the 21 joints into 128 numbers and classifies them: open palm, fist, victory, and four more.

---

<div class="tag">Hand tracking</div>

## MediaPipe hands on anira<br><span class="then">The camera thread pushes and never waits.</span>

<div class="statement-points">
  <div><small>1</small>One handler per model, one landmark handler per hand</div>
  <div><small>2</small>Every frame: pop what has finished, push the new crops</div>
  <div><small>3</small>The result reaches the GUI through a triple buffer</div>
  <div><small>4</small>The decoder's pool: 30 frames and 21.5 latent frames a second</div>
</div>

Note:
- The chain runs on anira, through the 2.x C++ API, which the v3 library still carries.
- Each model has its own handler, and each tracked hand has its own landmark handler, so two hands run in parallel.
- The camera thread never waits for an inference: each frame it pops what has finished and pushes new work.
- A result that is late arrives with the next frame; the tracking rate drops, the camera rate does not.
- The newest result goes to the GUI through a triple buffer, without a lock.
- The tracker and the decoder share the threads: camera frames and audio blocks in one pool.

---

<div class="tag">Goal</div>

## The goal<br><span class="then">The hands play the eight latents.</span>

<div class="statement-points">
  <div><small>1</small>Hands → roles: the right hand plays, the other one colours</div>
  <div><small>2</small>Features → latents: one row per latent, from feature 0 to feature 1</div>
</div>

Note:
- Two TODOs, both in HandControls.cpp.
- First, which tracked hand plays which role: the right hand leads, the left one sets the texture, and a single hand always leads.
- Second, the mapping table: which feature of which hand moves which latent, and over what range.

---

<div class="tag">Given</div>

## What we get<br><span class="then">5 models, 1 header, 1 file to edit.</span>

<div class="given">
  <div class="given-column many">
    <div class="given-label">Models</div>
    <div class="given-folder">common/assets/models/</div>
    {{FILE:model_decoder_noise.onnx|new}}
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
- The four hand models and the decoder come with the other models.
- The decoder is step 10's, but it fills the 120 latent dimensions the model leaves out with fresh noise instead of zeros: no reference to check against, a livelier sound.
- HandTracker.h is all you need from the hand tracking; the inference stays behind it.
- The two TODOs are in HandControls.cpp.
- It is main-thread code: the hand arrives 30 times a second, and the audio thread only reads.
- The camera comes from AVFoundation on macOS, JUCE on Windows and OpenCV on Linux; without a camera, an XY pad plays.
- The app needs the JUCE build, which takes a few minutes, so try it at home.

---

<div class="tag">Helpers: <code>HandTracker.h</code></div>

## The hand tracker<br><span class="then">Camera frames in, hands out.</span>

```cpp
struct HandFeatures { float x, y, openness, pinch, roll; };  // each 0 … 1

struct Hand {
    int id;                  // the same while the hand is tracked
    bool is_right;           // your right hand
    float confidence;
    HandFeatures features;
    const char* gesture;     // "Open_Palm", "Victory", …
    std::array<juce::Point<float>, 21> landmarks;  // for drawing
};

class HandTracker {
    void pushFrame(const juce::Image& frame);  // camera thread, never waits
    std::vector<Hand> hands();                 // GUI timer: up to two, most confident first
};
```

Note:
- This header is the whole interface of the hand tracking; anira and MediaPipe stay in HandTracker.cpp.
- Every feature is a number from 0 to 1, seen like a mirror: x is left to right, y bottom to top, openness fist to open palm, pinch apart to touching, roll tilted left to tilted right.
- is_right is your right hand, already mirrored.
- The landmarks are only for drawing the skeleton; the features are computed from them.

---

<div class="tag">Helpers: <code>HandTracker.h</code></div>

## The hand tracker<br><span class="then">Three calls from the camera to the latents.</span>

```cpp
HandTracker tracker;                               // loads the four models

// camera thread, every frame
tracker.pushFrame(image);

// GUI timer, 60 times a second
controls.update(tracker.hands(), dt, settings);    // TODO 1 and 2
decoder.setLatents(controls.latents());            // the audio thread reads them
```

Note:
- MainComponent does this for you; it is all the app does with the tracker.
- The camera thread hands every frame over and goes on.
- The GUI timer asks for the newest hands and gives them to HandControls, where your two TODOs are.
- The latents go to the decoder as atomics, and the audio thread reads them once per 2048 samples.

---

<div class="tag">Task: <code>HandControls.cpp</code></div>

## Task

<div class="task-timer" data-minutes="10"></div>

Fill in the TODOs in `steps/11_hands/exercise/HandControls.cpp`

```bash
cmake --build --preset release --target step11_exercise
open build/steps/11_hands/step11_exercise_artefacts/Release/*.app
```

<div class="run">
  <div class="run-step" data-seconds="2.6">
    <div class="run-title"><span>1</span>Hands → roles</div>
    <div class="run-visual run-setup">
      <div class="run-note">right hand → lead · the other → texture</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>2</span>Features → latents</div>
    <div class="run-visual run-setup">
      <div class="run-note">open → z0 · height → z2 · the rest by ear</div>
      <div class="run-ok">✓ the piano follows the hand</div>
    </div>
  </div>
</div>

Note:
- This is the task for at home, sized at ten minutes.
- Build step11_exercise and open the app; the README has the commands.
- First the roles: the tracker gives up to two hands, and you decide which one plays.
- Then the table: openness goes to z0, height to z2, and the rest you set by ear.
- It works when the piano follows the hand.

---

<div class="tag">Solution</div>

## TODO 1 · Hands → roles

```cpp
for (const Hand& hand : hands) {
    if (hand.is_right != settings.swap_hands && lead == nullptr)
        lead = &hand;
    else if (texture == nullptr)
        texture = &hand;
}
if (lead == nullptr)
    std::swap(lead, texture);  // a single hand always plays
```

Note:
- The hands come most confident first, so the first right hand leads.
- Any other hand colours; that also covers two hands the tracker calls the same side.
- A single left hand still plays: if there is no lead, the texture hand becomes it.
- swap_hands is the checkbox for left-handed players.

---

<div class="tag">Solution</div>

## TODO 2 · Features → latents

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

## Two models, three rates,<br><span class="then">one library.</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
- Two models, three rates, one library: the camera at 30 frames a second, the latents at one frame per 2048 samples, about 21.5 a second, and the audio at 44.1 kHz.
- One thing is still a copy: every camera frame goes to the CPU and then to the engine.
- That is where the outlook starts.
