# Step 10: Encoder, latents, decoder

**Goal:** the model cut in the middle. The encoder turns 2048 samples into eight numbers, the decoder turns eight numbers into 2048 samples, and between them is your code: a bias and a scale per dimension, the instrument.

**Problem it solves:** step 9 ran the model as a black box. Two handlers on one context make the middle reachable, and show what two streams at two rates need from each other: a latent frame per 2048 samples, and a latency that covers both.

## What you have

- `models/model_encoder.onnx`, `models/model_decoder.onnx` — the two halves, each with its own state pair (34,304 and 29,184 floats).
- [`exercise/encoder.json`](exercise/encoder.json) — audio in, one latent frame `[8, 1]` out per 2048 samples. Complete.
- [`exercise/decoder.json`](exercise/decoder.json) — a latent frame in, 2048 samples out. Its anchor is the audio output: that is its clock, the latent input follows at one frame per 2048 samples. The model's own delay, 2224, is declared here, once for the whole chain. Complete.
- [`exercise/contract.json`](exercise/contract.json) — the contract from step 9, used for both.
- `common/helpers/host.h`, `common/helpers/support.h` — as in step 9.

## What to do

Open [`exercise/main.cpp`](exercise/main.cpp). Three TODOs, each in two places: the planning on the main thread and the audio callback. Everything is `anira.hpp`; `create()` and `prepare()` are given helpers, and the blanks are marked `fill out`.

1. **The encoder.** It is created from `encoder.json`; prepare it and keep its latency: it counts in latent frames. In the callback, push the block (`encoder.push_data`); the loop pops frames while `encoder.get_available_samples` says one is waiting: a pop with nothing waiting is a miss, and the miss policy would fill it. The first `encoder_latency_frames` frames are its priming, zeros. A zero latent is not silence; it decodes to a sound of its own. Skip those.
2. **Bias and scale.** `frame_data[i] * scale[i] + bias[i]`, with the atomics loaded on the audio thread. With zero bias and unit scale, nothing changes, and the check passes.
3. **The decoder.** It is created and prepared; its latency counts in samples and assumes a frame is there when its window starts. Ours arrives late by the encoder's latency, so the latency the host is told is the sum: the encoder's frames times `k_frame_size`, plus the decoder's. Declare it on the decoder's output, `contract.hard_latency("audio", …)`, and prepare the decoder again. In the callback, push every real frame into the decoder (`decoder.push_data`) and pop its audio for this callback (`decoder.pop_data`).

```bash
cmake --build --preset release --target step10_exercise
./build/bin/step10_exercise
```

## Expected output

```
Reported latency: 8368 samples (189.8 ms at 44100 Hz)

  latent frames                8, 1 of them priming
OK: max abs diff 2.38e-07, within 0.0001 of the reference.
```

8368 is the encoder's one frame (2048) plus the decoder's 6320: the chain costs one more frame than the forward model in step 9, because the decoder cannot start before the encoder has delivered.

The check is the one from step 9. With the latents untouched, encoder and decoder together are the forward model to 3e-7.

## If you're stuck

- **`TODO 1: prepare the encoder and keep its latency.`** — `encoder_latency_frames` is still 0.
- **`TODO 3: the latency the host is told: ...`** — `latency` is still 0.
- **`TODO 3: declare the sum on the decoder's output and prepare the decoder again.`** — the decoder still reports its own latency: the frame would arrive after its window started.
- **`TODO 1: no latent frame came out of the encoder.`** — the block was not pushed into the encoder.
- **`TODO 3: the output is the input`** — the decoder's audio was not popped into the block.
- **`FAILED: max abs diff 0.137`**, no late callbacks — the priming frame reached the decoder. Skip the first `encoder_latency_frames` frames.
- **`FAILED: max abs diff 0.15`** — a state is not carried, or the frame goes into the wrong slot; both handlers' slot 0 is the streamed tensor.

Compare with [`solution/`](solution/) when you want to.

## Slides

[`slides/16-split.md`](../../slides/16-split.md).

## What's next

A hand instead of the encoder: the latents from a camera, the decoder as a generator.
