# Step 10: Encoder, latents, decoder

**Goal:** the model cut in the middle. The encoder turns 2048 samples into eight numbers, the decoder turns eight numbers into 2048 samples, and between them is your code: an offset per dimension, the instrument, as in anira's own example.

**Problem it solves:** step 9 ran the model as a black box. Two handlers on one context make the middle reachable, and show what two streams at two rates need from each other: a latent frame per 2048 samples, and a latency that covers both.

## What you have

- `steps/common/assets/models/rave/model_encoder.onnx`, `model_decoder.onnx` — the two halves, each with its own state pair (34,304 and 29,184 floats).
- [`exercise/encoder.json`](exercise/encoder.json) — audio in, one latent frame `[8, 1]` out per 2048 samples. Complete.
- [`exercise/decoder.json`](exercise/decoder.json) — a latent frame in, 2048 samples out. Its anchor is the audio output: that is its clock, the latent input follows at one frame per 2048 samples. The model's own delay, 2224, is declared here, once for the whole chain. Complete.
- [`exercise/contract.json`](exercise/contract.json) — like step 9's, but `on_miss: zeros` (bypass needs an input of the output's shape), used for both.
- [`source/SplitProcessor.h`](source/SplitProcessor.h) — the class you complete: `prepare` before the stream, `process` per callback, `set_offset` per knob.
- [`source/PluginProcessor.cpp`](source/PluginProcessor.cpp) — the plugin around it, step 9's with eight knobs, z0 to z7, from −1 to 1 as in anira's example. Complete.
- [`check.cpp`](check.cpp) — drives `SplitProcessor` on the fake host from step 9 and compares the output with the reference. Complete.

## What to do

Open [`exercise/SplitProcessor.cpp`](exercise/SplitProcessor.cpp). Four TODO banners, each in one place, in the order the code runs; the blanks are marked `fill out`. Creating and preparing the handlers is given. The structure is the one of anira's own encoder/decoder example (`examples/juce-audio-plugin`, model 7): host blocks from 1 up to the host's largest, one latent frame per 2048 input samples, and the host told the encoder's latency plus the decoder's.

1. **The latency** (`prepare`). Both latencies are read for you, each in the unit of its handler's output: the encoder's in latent frames, the decoder's in samples. The host is told samples: the encoder's frames times `k_frame_size`, plus the decoder's.
2. **The encoder** (`process`). Push the block (`m_encoder->push_data`). The loop counts the input samples and pops exactly one frame per 2048 into `m_frame` (`m_encoder->pop_data`): 44100 / 2048, about 21.5 latent frames a second, the latents' own rate. A frame that is not there yet is a miss: the miss policy fills it, it keeps its place in the stream, and anira drops the late one when it arrives. The first `m_encoder_latency_frames` frames are its priming, zeros; they go on to the decoder like every other frame.
3. **An offset on z** (`process`). `m_frame_data[i] += m_offset[i]`, with the atomics loaded on the audio thread. With all offsets at zero nothing changes and the comparison passes; the check then sets z0 to 1, and the output has to change.
4. **The decoder** (`process`). Push every frame into the decoder (`m_decoder->push_data`), and once per callback pop its audio into the block (`m_decoder->pop_data`).

Build and run from the repository root:

```bash
cmake --build --preset release --target step10_exercise_check
./build/bin/step10_exercise_check
```

Once it passes, play: the same `SplitProcessor` as a plugin, with the eight knobs.

```bash
cmake --build --preset release --target step10_exercise_Standalone   # or step10_exercise_VST3
```

The result is in `build/steps/10_split/step10_exercise_artefacts/Release/`. A double-click on a knob sets it back to 0.

## Expected output

```
Reported latency: 8368 samples (189.8 ms at 44100 Hz)

  latent frames                8, 1 of them priming
OK: max abs diff 1.71e-07, within 0.0001 of the reference.
OK: an offset of 1 on z0 changes the output.
```

8368 is the encoder's one frame (2048) plus the decoder's 6320: the chain costs one more frame than the forward model in step 9, because the decoder cannot start before the encoder has delivered.

The check compares against step 10's own reference, [`target_split_signal.h`](../common/assets/target_split_signal.h): encoder and decoder as anira's example runs them, the priming frame included. That zero frame changes the decoder's state, so this output differs from the forward model's by about 0.014; `scripts/make_test_data.py` writes both references.

## If you're stuck

- **`TODO 1: the latency the host is told: ...`** — `m_latency` is still 0.
- **`TODO 2: no latent frame came out of the encoder.`** — the block was not pushed into the encoder, or the frame was not popped (`popped = m_encoder->pop_data(m_frame, 0)`).
- **`TODO 4: the output is the input`** — the decoder's audio was not popped into the block.
- **`TODO 3: an offset on z0 does not change the output`** — the offsets are not added to `m_frame_data`.
- **`FAILED: max abs diff 0.15`** — a state is not carried, or the frame goes into the wrong slot; both handlers' slot 0 is the streamed tensor.

Compare with [`solution/`](solution/) when you want to.

## Slides

[`slides/16-split.md`](../../slides/16-split.md).

## What's next

A hand instead of the encoder: the latents from a camera, the decoder as a generator.
