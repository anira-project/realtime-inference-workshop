# Step 10: The model as an instrument

**Goal:** a camera instead of the encoder. Hand tracking and the decoder run in one anira pool, two models of two kinds at three rates (camera frames, latent frames, audio samples), and your hand drives the latents.

**Problem it solves:** the model has an encoder and a decoder, eight numbers between them (the bonus step opens it up). Here the encoder is gone: the latents come from somewhere else entirely, and the decoder runs as a generator, with no audio in.

## What you have

A JUCE app (JUCE is built once, the first time, a few minutes):

```bash
cmake --preset release
cmake --build --preset release --target step10_solution
open "build/release/steps/10_hands/step10_solution_artefacts/Release/Workshop Hands (solution).app"
```

- [`../common/hands/`](../common/hands/) — the camera and the hand tracking, given and hidden: you never open them. [`HandTracker.h`](../common/hands/HandTracker.h) is the one header worth a look. It gives you up to two hands, each with five features from 0 to 1 (left-right, height, open, pinch, tilt), which hand it is, a gesture, and 21 landmarks for drawing.
- Behind it runs MediaPipe's hand chain from [anira-cam](https://github.com/anira-project/anira-cam): palm detector, landmarks, gesture. It uses anira's v3 API like the decoder, one model config and a hard contract per model, on the same context. Its four ONNX models (Apache 2.0) come with the other models and are compiled into the app. The camera: AVFoundation on macOS, `juce::CameraDevice` on Windows, OpenCV on Linux (`sudo apt install libopencv-dev` before configuring).
- Without a camera the video area is a pad, and the mouse is the lead hand: drag to move it, hold the button to open the hand, right button to pinch.
- [`source/HandControls.cpp`](source/HandControls.cpp) — which hand leads (your right one; the other sets the texture), your `map_hands`, the smoothing. Given.
- [`source/LatentDecoder.cpp`](source/LatentDecoder.cpp) — the decoder from the bonus step as a generator. No audio goes in: every 2048 samples, the audio thread hands it the current latents as one frame and pulls one model block of audio out. Resampled to the device's rate.
- [`decoder.json`](decoder.json), [`contract.json`](contract.json) — the same decoder as in the bonus step, `model_decoder.onnx`. It fills the 120 latent dimensions the model leaves out with zeros, as in the bonus step.

## What to do

The fun part: no TODO, no check. Build the app, open it, play. [`solution/HandMapping.cpp`](solution/HandMapping.cpp) holds the one mapping from the hands to the latent vector: open is loud, high is bright, a second hand adds texture. Change it, rebuild, play.

`map_range(feature, at_0, at_1)` is the straight line from `at_0` (feature 0) to `at_1` (feature 1). z0 is the loudness (+1 is about +5.5 dB, −3 close to silent), z2 the brightness; the other six you find by ear. About ±2 is each dimension's natural range.

## What to look at while it runs

- The status line: camera and tracking rate, and the decoder's latency. Two models, one pool: the tracker's jobs and the decoder's blocks share the threads, and neither waits for the other.
- The latent panel: the smoothed value and the target tick of each latent.
- Without a hand the latents glide to rest and the sound fades (`Silence without hands`).

## If you're stuck

- **The left hand plays** — `is_right` is your right hand; `Swap hands` flips it.
- **`The decoder failed to load: ...`** — the message names the file or tensor; `decoder.json` points to `model_decoder.onnx`, which every configure downloads.
- **macOS: the video area stays empty or says the camera was denied** — start the app with `open` (or a double-click), not the binary inside `Contents/MacOS/`, and not from an IDE: started from a terminal or IDE, macOS asks whether that program may use the camera, not the app. Allow it in *System Settings → Privacy & Security → Camera* if you refused once.
- **`No camera found.` / `No camera on this platform.`** — the pad plays. On macOS, allow the camera when asked; the app has to be restarted after a refusal. On Linux, install `libopencv-dev` and configure again, and be in the `video` group.
- **Loud, strange sound** — a latent far outside about ±3; the natural range of each is about ±2.

## Slides

[`slides/16-hands.md`](../../slides/16-hands.md).

## What's next

Every camera frame still travels camera → CPU → engine as a copy. That is the outlook: the v3 release with GPU and NPU memory, where the frame stays where it is.
