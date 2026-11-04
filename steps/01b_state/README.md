# Step 1b: Stateless and stateful

**Goal:** run the whole test signal through the model, by passing the audio and the state.

## What you have

The same model, signals and engine as in [step 1a](../01a_minimal_inference/README.md).

The model takes more than audio, and returns more than audio:

```
audio[1,1,2048], state_in[1,63488] -> audio_out[1,1,2048], state_out[1,63488]
```

The state is every convolution cache in the model, and zeros start a new stream. The engine takes it as the second buffer, and overwrites it on every call:

```cpp
// {audio, state}:          the two buffers
// {samples, state size}:  their sizes
engine.process({audio, state}, {num_samples, state_size}, start);
```

## What to do

Build and run the exercise as it is, and look at the blocks:

```bash
cmake --preset release
cmake --build --preset release --target step01b_exercise
./build/bin/step01b_exercise
```

Then open [`exercise/main.cpp`](exercise/main.cpp) and fill in the two TODO banners: create the state above the loop, and pass the audio, the state and their sizes in the loop. The state is 63488 floats, starting at zero.

## Expected output

```
  block 0                  max abs diff 2.33e-10   OK
  block 1                  max abs diff 1.12e-07   OK
  ...
  block 7                  max abs diff 3.87e-07   OK
OK: max abs diff 3.87e-07, within 0.0001 of the reference.
```

## If you're stuck

- **`TODO 1 and 2: block 0 is right, the blocks after it are not`** — the model only gets the audio, so every block starts from a zero state: every block is the first block.
- **`FAILED: got 2 buffers but 1 sizes: ...`** — the sizes need the size of the state as well.
- **`FAILED: the state is exactly 63488 floats, ...`** — the state buffer or its size is wrong.
- **The same failure after adding the state** — the state is being created inside the loop, so it starts at zero on every block. It belongs above the loop.

Compare with [`solution/main.cpp`](solution/main.cpp) when you want to.

## Slides

[`slides/01b-state.md`](../../slides/01b-state.md), or the deployed deck.

## What's next?

The numbers are right, as long as the host hands us 2048 samples. Step 2 deals with hosts that don't.
