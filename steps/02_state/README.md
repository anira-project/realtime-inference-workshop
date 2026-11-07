# Step 2: Stateless and stateful

**Goal:** run the whole test signal through the model, by passing the audio and the state.

## What you have

The same model, signals and engine as in [step 1](../01_minimal_inference/README.md).

The model takes more than audio, and returns more than audio:

```
audio[1,1,2048], state_in[1,63488] -> audio_out[1,1,2048], state_out[1,63488]
```

The state is every convolution cache in the model, and zeros start a new stream. The engine takes it as two more arguments, where it starts and how many floats it has, and overwrites it on every call:

```cpp
engine.process(audio, num_samples);                     // Without the state: every call starts from zeros
engine.process(audio, num_samples, state, state_size);  // With it: the call continues where the last one stopped
```

## What to do

Build and run the exercise as it is, and look at the blocks:

```bash
cmake --preset release
cmake --build --preset release --target step02_exercise
./build/release/bin/step02_exercise
```

Then open [`exercise/main.cpp`](exercise/main.cpp) and fill in the two TODO banners: make a vector for the state above the loop, and pass it to `process` in the loop. The model writes the state on every call; the vector is only the memory it writes into: 63488 floats, starting at zero.

## Expected output

```
  PASS  block 0    max abs diff 2.33e-10
  PASS  block 1    max abs diff 1.27e-07
  ...
  PASS  block 7    max abs diff 3.58e-07
  PASS  the output matches the reference: max abs diff 3.87e-07, tolerance 0.0001

==============================================================
  STEP 2 SOLVED
==============================================================
```

## If you're stuck

The exercise builds once every `?` is filled in. Then each TODO says so while it is off:

- **`TODO 1: the state has N floats, the model's state is 63488`** — the state vector has the wrong size. The check looks before the first block: the engine only gets a pointer and a size, and cannot see how large the vector really is.
- **`TODO 1: the state starts at zero, before the first block`** — the state vector is filled with something else.
- **`TODO 2: block 0 is right, the blocks after it are not`** — the model only gets the audio, so every block starts from a zero state: every block is the first block.
- **`the state is exactly 63488 floats, got ...`** — TODO 2 passes the wrong size next to the state.

Compare with [`solution/main.cpp`](solution/main.cpp) when you want to.

## Slides

[`slides/02-state.md`](../../slides/02-state.md), or the deployed deck.

## What's next?

The numbers are right, as long as the host hands us 2048 samples. Step 3 deals with hosts that don't.
