# Step 10: Opening the model

**Goal:** the model cut in the middle. The encoder turns 2048 samples into eight numbers, the decoder turns eight numbers into 2048 samples. Between them is your code: an offset per dimension, as in anira's own encoder/decoder example. Eight knobs on the model, an instrument.

**Problem it solves:** step 9 ran the model as a black box. Split into its two halves, the middle becomes reachable: eight numbers per block that describe how it sounds, and that you can change before the decoder makes sound from them.

## What you have

- `steps/common/assets/models/rave/model_encoder.onnx`, `model_decoder.onnx` — the two halves of the model, each with its own state pair (34,304 and 29,184 floats; together the forward model's 63,488).
- [`exercise/encoder.json`](exercise/encoder.json), [`exercise/decoder.json`](exercise/decoder.json) — the two model files, every key in place; the state pairs are given. Every `?` is a blank, with a hint next to it.
- [`exercise/contract.json`](exercise/contract.json) — like step 9's, but `on_miss: zeros`, as in anira's encoder/decoder example: bypass passes the input through, and anira refuses it when the output has other channels than the input (8 latents from 1 audio channel). Used for both handlers.
- [`exercise/SplitProcessor.cpp`](exercise/SplitProcessor.cpp) — everything given but one line: the two handlers, `prepare`, the latency, the frames passed from the encoder to the decoder, and the offline case.
- [`source/PluginProcessor.cpp`](source/PluginProcessor.cpp) — the plugin around it, step 9's with eight knobs, z0 to z7, from −3 to 3. It hands them to `process` as `offsets` in every block. Complete.
- [`check.cpp`](check.cpp) — drives `SplitProcessor` offline, as a bounce, and compares the output with the reference. Complete.

## What to do

1. **Declare the tensors** (`encoder.json`, `decoder.json`). Replace every `?` with its number. The encoder takes one model block of audio, `time` 2048 as in step 9, and gives the latents: `channel` 8, `time` 1, eight latents in one frame per inference. The decoder takes those latents and gives one model block back.
2. **An offset on z** (`SplitProcessor.cpp`). One line, marked `fill out`: add each dimension's offset to its latent, before the frame goes on to the decoder. `offsets` is a parameter of `process`: the plugin hands in its eight knobs.

Build and run from the repository root:

```bash
cmake --build --preset release --target step10_exercise_check
./build/release/bin/step10_exercise_check
```

Once it passes, play: the plugin, eight knobs, one per latent.

```bash
cmake --build --preset release --target step10_exercise_Standalone   # or step10_exercise_VST3
```

The result is in `build/release/steps/10_split/step10_exercise_artefacts/Release/`. A double-click on a knob sets it back to 0. Turn one at a time: in this model z0 changes the loudness and z2 the brightness. **report** shows step 9's report, one card for the encoder and one for the decoder; a slow machine and a rate other than 44.1 kHz ask first, as in step 9.

## Expected output

```
anira's report, encoder (at prepare, on this machine):
  budget 6.1 ms   measured 6.1 ms   median 6.1 ms
  warm-up 4 inferences, until stable
  load 13 % (real-time factor 0.13 over 1 inference): comfortable

anira's report, decoder (at prepare, on this machine):
  budget 6.3 ms   measured 6.3 ms   median 6.0 ms
  warm-up 6 inferences, until stable
  load 14 % (real-time factor 0.14 over 1 inference): comfortable

  reported latency             6873 samples (155.9 ms at 44100 Hz)
  latent frames                8, 1 of them priming
  PASS  the output matches the reference: max abs diff 2.31e-07, tolerance 0.0001
  PASS  the offsets change the output

==============================================================
  STEP 10 SOLVED
==============================================================
```

The latency is this machine's: both budgets are measured at prepare. The check runs offline, as a bounce: every block waits for its result, so nothing is missed and the comparison is exact. It compares against step 10's own reference, [`target_split_signal.h`](../common/assets/target_split_signal.h): encoder and decoder as anira's example runs them, the encoder's priming frame included. That frame changes the decoder's state, so the output differs a little from the forward model's. Step 9's reference would not match.

## If you're stuck

`SplitProcessor.cpp` builds once its `?` is filled in. The `?` in the JSON files are found when the check runs:

- **`encoder.json, line 12: ? is still open`** — a blank not filled yet; the line says which.
- **`"8" is text; write the number without quotes`** — a number in quotes is a string in JSON.
- **`onnxruntime: output tensor 'latents' bound to the model's 'latents': the model's tensor is [1, 8, 1], the spec's engine dims are [1, 7, 1]`** — a size is filled but wrong; prepare checks every tensor against the model file and names the one that does not fit, with both shapes.
- **`TODO 2: the offsets do not change the output`** — `offsets[i]` is not added to `m_frame_data[i]`.

Compare with [`solution/`](solution/) when you want to.

## Slides

[`slides/16-split.md`](../../slides/16-split.md).

## What's next

A hand instead of the encoder: the latents from a camera, the decoder as a generator.
