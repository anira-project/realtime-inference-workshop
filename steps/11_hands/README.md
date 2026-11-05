# Step 11: Hands on the latents

**Goal:** a camera instead of the encoder. Hand tracking and the decoder run in one anira pool, two models of two kinds at three rates (camera frames, latent frames, audio samples), and the hand drives the eight latents.

**Problem it solves:** step 10 put your code between encoder and decoder. Here the encoder is gone: the latents come from somewhere else entirely, and the decoder runs as a generator, with no audio in.

## What you have

A JUCE app (JUCE is built once, the first time, a few minutes):

```bash
cmake --preset release
cmake --build --preset release --target step11_exercise      # or step11_solution
open "build/steps/11_hands/step11_exercise_artefacts/Release/Workshop Hands (exercise).app"
```

- [`source/HandTracker.h`](source/HandTracker.h) — the hand tracking, in one header: camera frames in, up to two hands out, each with its features in [0, 1] (left-right, height, open, pinch, tilt), which hand it is, a gesture and 21 landmarks for drawing. Given.
- [`handtracking/`](handtracking/) and [`source/HandTracker.cpp`](source/HandTracker.cpp) — behind that header: MediaPipe's hand chain on anira, from [anira-cam](https://github.com/anira-project/anira-cam) (palm detector, landmarks, gesture), on anira's 2.x C++ API, which the v3 library still carries. Its four ONNX models (`hand_detector.onnx` and friends, Apache 2.0) come with the other models and are compiled into the app. You don't need to open it.
- [`source/LatentDecoder.cpp`](source/LatentDecoder.cpp) — the decoder from step 10 as a generator: the audio thread hands it one frame of the current latents per 2048 samples it pulls out, two frames ahead. Resampled to the device's rate.
- [`source/HandControls.h`](source/HandControls.h) — roles (the lead hand plays, the texture hand colours), the mapping from features to latents, the smoothing. Given, except the table.
- [`source/CameraInput.*`](source/) — AVFoundation on macOS, `juce::CameraDevice` on Windows, OpenCV on Linux (JUCE has no camera there; `sudo apt install libopencv-dev` before configuring). Without a camera the video area is a pad, the mouse is the lead hand (drag; hold the button to open the hand, right button to pinch).
- [`decoder.json`](decoder.json), [`contract.json`](contract.json) — from step 10, with one change: the decoder is `model_decoder_noise.onnx`. Step 10's decoder fills the 120 latent dimensions the model leaves out with zeros, so its output can be checked against a reference; this one fills them with fresh noise every block, as the model was trained, and sounds livelier.

## What to do

Open [`exercise/HandControls.cpp`](exercise/HandControls.cpp) and [`source/HandTracker.h`](source/HandTracker.h) next to it.

**TODO 1 · Hands → roles.** `update()` gets what `HandTracker::hands()` returned: up to two hands, the most confident first. Point `lead` at the hand that plays (`is_right != settings.swap_hands`, or the only hand) and `texture` at the other one. Until then no hand plays, and the app says so in a banner.

**TODO 2 · Features → latents.** The mapping table: one row per latent, with which hand, which feature, the latent at feature 0 and at feature 1, the value at rest. Every range is empty, so the hand changes nothing, and the app says so in a banner.

Start with z0 (dynamics: fist quiet, open loud: `-2.5` to `2.0`, rest `-3`) and z2 (register: low to high, `-2.5` to `3.0`). Then the rest, by ear. The natural range of each dimension is about ±2; the solution's table and its reasons are in [`solution/HandControls.cpp`](solution/HandControls.cpp).

## What to look at while it runs

- The status line: camera and tracking rate, and the decoder's latency. Two models, one pool: the tracker's jobs and the decoder's blocks share the threads, and neither waits for the other.
- The latent panel: the mapped range of each dimension, the smoothed value, the target tick.
- Without a hand the latents glide to rest and the sound fades (`Silence without hands`).

## If you're stuck

- **Banner `TODO 1: the tracker sees a hand, but no hand plays`** — `lead` is still `nullptr` after TODO 1.
- **Banner `TODO 2: every range in the mapping table is empty`** — fill in `at_0` and `at_1`.
- **The left hand plays** — `is_right` is your right hand; `swap_hands` flips it.
- **`The decoder failed to load: ...`** — the message names the file or tensor; `decoder.json` points to `model_decoder_noise.onnx`, which a configure with anira and JUCE on downloads.
- **`No camera found.` / `No camera on this platform.`** — the pad plays. On macOS, allow the camera when asked; the app has to be restarted after a refusal. On Linux, install `libopencv-dev` and configure again, and be in the `video` group.
- **Loud, strange sound without a hand** — a latent's rest value far from the model's quiet region; z0 at rest belongs near −3.

## Slides

[`slides/17-hands.md`](../../slides/17-hands.md).

## What's next

Every camera frame still travels camera → CPU → engine as a copy. That is the outlook: the v3 release with GPU and NPU memory, where the frame stays where it is.
