# Step 11: Hands on the latents

**Goal:** a camera instead of the encoder. Hand tracking and the decoder run in one anira pool, two models of two kinds at two rates, and the hand drives the eight latents.

**Problem it solves:** step 10 put your code between encoder and decoder. Here the encoder is gone: the latents come from somewhere else entirely, and the decoder runs as a generator, with no audio in.

## What you have

A JUCE app, built with `-DWORKSHOP_JUCE=ON` (JUCE is cloned and built once, a few minutes):

```bash
cmake -S . -B build-juce -DCMAKE_BUILD_TYPE=Release -DWORKSHOP_JUCE=ON
cmake --build build-juce --target step11_exercise      # or step11_solution
open "build-juce/steps/11_hands/step11_exercise_artefacts/Release/Workshop Hands (exercise).app"
```

- [`handtracking/`](handtracking/) — MediaPipe's hand chain on anira, from [anira-cam](https://github.com/anira-project/anira-cam): palm detector, landmarks, gesture. Given; it runs on anira's 2.x C++ API, which the v3 library still carries. Its four ONNX models (`hand_detector.onnx` and friends, Apache 2.0) come with the other models and are compiled into the app.
- [`source/LatentDecoder.cpp`](source/LatentDecoder.cpp) — the decoder from step 10 as a generator: the audio thread hands it one frame of the current latents per 2048 samples it pulls out, two frames ahead. Resampled to the device's rate.
- [`source/HandControls.h`](source/HandControls.h) — features of a hand (left-right, height, open, pinch, tilt), roles (the lead hand plays, the texture hand colours), the smoothing. Given.
- [`source/CameraInput.*`](source/) — AVFoundation on macOS, `juce::CameraDevice` on Windows. Linux has no camera in JUCE: the video area is a pad, the mouse is the lead hand (drag; hold the button to open the hand, right button to pinch).
- [`decoder.json`](decoder.json), [`contract.json`](contract.json) — from step 10.

## What to do

Open [`exercise/HandControls.cpp`](exercise/HandControls.cpp): the mapping table at TODO 1. One row per latent: which hand, which feature, the latent at feature 0 and at feature 1, the value at rest. Every range is empty, so the hand changes nothing, and the app says so in a banner.

Start with z0 (dynamics: fist quiet, open loud: `-2.5` to `2.0`, rest `-3`) and z2 (register: low to high, `-2.5` to `3.0`). Then the rest, by ear. The natural range of each dimension is about ±2; the solution's table and its reasons are in [`solution/HandControls.cpp`](solution/HandControls.cpp).

## What to look at while it runs

- The status line: camera and tracking rate, and the decoder's latency. Two models, one pool: the tracker's jobs and the decoder's blocks share the threads, and neither waits for the other.
- The latent panel: the mapped range of each dimension, the smoothed value, the target tick.
- Without a hand the latents glide to rest and the sound fades (`Silence without hands`).

## If you're stuck

- **Banner `TODO 1: every range in the mapping table is empty`** — fill in `at_0` and `at_1`.
- **`The decoder failed to load: ...`** — the message names the file or tensor; `decoder.json` and `contract.json` are the ones from step 10.
- **`No camera found.` / `No camera on this platform.`** — the pad plays. On macOS, allow the camera when asked; the app has to be restarted after a refusal.
- **Loud, strange sound without a hand** — a latent's rest value far from the model's quiet region; z0 at rest belongs near −3.

## Slides

[`slides/15-hands.md`](../../slides/15-hands.md).

## What's next

Every camera frame still travels camera → CPU → engine as a copy. That is the outlook: the v3 release with GPU and NPU memory, where the frame stays where it is.
