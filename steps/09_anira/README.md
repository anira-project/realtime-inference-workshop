# Step 9: The model with anira

**Goal:** run the part 1 model on anira. Two JSON files say what the model is and how it runs; three calls connect it to the host. Steps 3, 6 and 7 are the library's now.

**Problem it solves:** everything from part 1 was infrastructure, and every piece of it is a place to get it wrong. Here it is declared, not implemented.

## What you have

- `steps/common/assets/models/rave/model_forward.onnx` — the model as in step 1: audio plus a state tensor in, audio plus the new state out.
- [`exercise/model.json`](exercise/model.json) — the model file, every key in place: the tensors are named, their roles and windows given. Every `"TODO 1"` and `"TODO 2"` is a blank: replace it with the value.
- [`exercise/contract.json`](exercise/contract.json) — how it runs: a budget of 20 ms per inference, with headroom (anira will measure it at prepare, `"budget": "measured"`, from the next alpha on; this pre-release needs a number), three warm-up runs, zeros on a miss. Complete.
- [`common/helpers/host.h`](../common/helpers/host.h) — the fake host from step 6, pacing its callbacks.
- [`common/helpers/support.h`](../common/helpers/support.h) — `report_with_latency`: the step 7 comparison, with the late callbacks left out.
- anira, fetched and built by CMake ([`cmake/anira.cmake`](../../cmake/anira.cmake)), with ONNX Runtime built in. The first configure clones it and builds it once; a couple of minutes.

## What to do

Open [`exercise/model.json`](exercise/model.json) and [`exercise/AniraProcessor.cpp`](exercise/AniraProcessor.cpp).

1. **Declare the tensors** (`model.json`). The sizes: `"time"` of the audio in and out is the model's block, 2048 (step 1). The state's length, 63488 floats, is given. The audio output carries the model's own delay: `"latency": 2224` (step 7).
2. **Pair the state** (`model.json`). `state_in` is fed from `state_out`: `"state_source": "state_out"`. That is the whole state loop from step 2.
3. **Prepare and process** (`AniraProcessor.cpp`). Loading, the pipeline, the handler and prepare are given, all from `anira.hpp`; three blanks, marked `fill out`:
   1. `m_contract.hard_host_blocks(…)`: the smallest block the host may send (any, down to 1), its largest (`max_block_size`) and its sample rate.
   2. `latency`: `handler.get_latency` of output slot 0.
   3. In the audio callback, `handler.process` over the block: in place, so the block is both input and output.

Build and run from the repository root:

```bash
cmake --build --preset release --target step09_exercise_check
./build/bin/step09_exercise_check
```

Once it passes, the same `AniraProcessor` runs as a plugin:

```bash
cmake --build --preset release --target step09_exercise_Standalone   # or step09_exercise_VST3
```

The result is in `build/steps/09_anira/step09_exercise_artefacts/Release/`.

## Expected output

```
Reported latency: 6320 samples (143.3 ms at 44100 Hz)

OK: max abs diff 2.79e-07, within 0.0001 of the reference.
```

6320, because the contract declares host blocks from 1 to 512: the host may call with any block up to its largest, so the latency covers the worst one, whatever the budget. The 20 ms budget in `contract.json` is a placeholder until anira measures the budget at prepare.

The check is step 7's: the output against the reference, shifted by the reported latency less the model's own 2224, which the reference already contains. A callback whose block was not back in time went out as zeros (`ANIRA_MISSED`); those are listed and left out, and more than a quarter of them fails the run. On a busy machine a few are normal.

## If you're stuck

- **`inputs[0].axes[2][1]: an extent is a positive integer or "dynamic"`** — TODO 1: a size is still `"TODO 1"`. The path names it: `inputs[0]` is `audio`, `inputs[1]` is `state_in`, `outputs[…]` the same for the outputs.
- **`prepare: ... 'audio' ... axes`** or a similar refusal naming a tensor — TODO 1: a size is filled but wrong; prepare checks every spec against the file and says which one does not fit.
- **`create: configuration error: tensor 'state_in': a State input needs a state_source`** — TODO 2: a state input nobody feeds is refused by name, at create.
- **`prepare: configuration error: contract: Hard host blocks missing`** — TODO 3.1: the contract does not know the host's blocks yet.
- **`TODO 3: read the latency of output slot 0`** — TODO 3.2.
- **`outputs[0].latency: an integer is required, got "TODO 1"`** — TODO 1: the latency of `audio_out`.
- **`state_source 'TODO 2' names no output tensor`** — TODO 2.
- **`TODO 1: audio_out declares no latency`** — `"latency"` in `model.json` is 0.
- **`TODO 3: the output is the input`** — TODO 3.3: the block goes out untouched; nothing processed it.
- **`FAILED: max abs diff 0.158`** with the latency reported — the state is not carried: block 0 matches, every block after it does not. Same failure as a fresh engine per block in step 1. Check `state_source`.
- **`prepare: not supported in this build: contract: a MEASURED budget ...`** — this anira build needs an explicit budget; keep `{"ms": 20}`.
- **Many late callbacks** — the machine is busy, or 20 ms is below its worst case. Raise the budget; the latency grows with it.

Compare with [`solution/`](solution/) when you want to.

## Slides

[`slides/15-anira-inference.md`](../../slides/15-anira-inference.md).

## What's next

The model taken apart: encoder and decoder as two handlers, and eight numbers between them you can reach.
