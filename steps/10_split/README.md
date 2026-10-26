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

Open [`exercise/main.cpp`](exercise/main.cpp). Three TODOs, each in two places: the planning on the main thread and the audio callback.

1. **The encoder.** Create it from `encoder.json`, prepare it, keep its latency: it counts in latent frames. In the callback, push the block and pop frames while one is delivered. The first `encoder_latency_frames` frames are its priming, zeros. A zero latent is not silence; it decodes to a sound of its own. Skip those.
2. **Bias and scale.** `frame_data[i] * scale[i] + bias[i]`, with the atomics loaded on the audio thread. With zero bias and unit scale, nothing changes, and the check passes.
3. **The decoder.** Create and prepare it; its latency counts in samples and assumes a frame is there when its window starts. Ours arrives late by the encoder's latency, so declare the sum on the decoder's output, `contract.hard_latency("audio", …)`, and prepare again. In the callback, push every real frame into the decoder and pop its audio for this callback.

```bash
cmake --build --preset release --target step10_exercise
./build/bin/step10_exercise
```

## Expected output

```
Reported latency: 6320 samples (143.3 ms at 44100 Hz)

  latent frames                8, 1 of them priming
OK: max abs diff 2.79e-07, within 0.0001 of the reference.
```

6320 is the encoder's one frame (2048) plus the decoder's 4272, the same figure as step 7's: the chain costs one more frame than the forward model in step 9, because the decoder cannot start before the encoder has delivered.

The check is the one from step 9. With the latents untouched, encoder and decoder together are the forward model to 3e-7.

## If you're stuck

- **`TODO 1: create and prepare the encoder first.`** / **`TODO 3: ...`** — the handlers are still null, or the latency is still 0.
- **`TODO 1: no latent frame came out of the encoder.`** — nothing was popped: the push, the pop loop, or `delivered` is not checked.
- **Late callbacks on every run, and `FAILED`** — the decoder was prepared once, with its own latency: the frame arrives after its window started, every time. Declare the sum and prepare again.
- **`FAILED: max abs diff 0.0137`**, no late callbacks — the priming frame reached the decoder. Skip the first `encoder_latency_frames` frames.
- **`FAILED: max abs diff 0.15`** — a state is not carried, or the frame goes into the wrong slot; both handlers' slot 0 is the streamed tensor.

Compare with [`solution/`](solution/) when you want to.

## Slides

[`slides/15-split.md`](../../slides/15-split.md).

## What's next

A hand instead of the encoder: the latents from a camera, the decoder as a generator.
