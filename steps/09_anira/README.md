# Step 9: The model with anira

**Goal:** run the part 1 model on anira. Two JSON files say what the model is and how it runs; three calls connect it to the host. Steps 2, 6 and 7 are the library's now.

**Problem it solves:** everything from part 1 was infrastructure, and every piece of it is a place to get it wrong. Here it is declared, not implemented.

## What you have

- `models/model_forward.onnx` — the model as in step 5: audio plus a state tensor in, audio plus the new state out.
- [`exercise/model.json`](exercise/model.json) — the model file, with gaps: the tensors are named, their shapes and roles are the task.
- [`exercise/contract.json`](exercise/contract.json) — how it runs: a budget of 8.2 ms per inference (the number from step 3), three warm-up runs, zeros on a miss. Complete.
- [`common/helpers/host.h`](../common/helpers/host.h) — the fake host from step 6, pacing its callbacks.
- [`common/helpers/support.h`](../common/helpers/support.h) — `report_with_latency`: the step 7 comparison, with the late callbacks left out.
- anira, fetched and built by CMake ([`cmake/anira.cmake`](../../cmake/anira.cmake)), with ONNX Runtime built in. The first configure clones it and builds it once; a couple of minutes.

## What to do

Open [`exercise/model.json`](exercise/model.json) and [`exercise/main.cpp`](exercise/main.cpp).

1. **Declare the tensors** (`model.json`). Every tensor needs its `axes` — `[["batch", 1], ["channel", 1], ["time", 2048]]` for the audio, `[["batch", 1], ["any", 63488]]` for the state — and the streamed ones their `window`. The output carries the model's own delay: `"latency": 2224`.
2. **Pair the state** (`model.json`). `state_in` is fed from `state_out`: `"state_source": "state_out"`. That is the whole state loop from step 5.
3. **Prepare and process** (`main.cpp`). Load the two files, give the contract the host's geometry, create the handler over a pipeline with one inference stage, prepare, read the latency. The audio callback is written: one tensor over the block, one call.

Build and run from the repository root:

```bash
cmake --build --preset release --target step09_exercise
./build/bin/step09_exercise
```

## Expected output

```
Reported latency: 4272 samples (96.9 ms at 44100 Hz)

OK: max abs diff 3.87e-07, within 0.0001 of the reference.
```

4272 is 1536 (block adaptation, 2048 − gcd(512, 2048)) + 512 (one host block for the 8.2 ms inference) + 2224 (the model). Step 7 reported 6320 for the same guarantee: the second whole block we gave the worker was more than the budget needed.

The check is step 7's: the output against the reference, shifted by the reported latency less the model's own 2224, which the reference already contains. A callback whose block was not back in time went out as zeros (`ANIRA_MISSED`); those are listed and left out, and more than a quarter of them fails the run. On a busy machine a few are normal.

## If you're stuck

- **`TODO 3: create and prepare the handler first.`** — `handler.m_handle` is still null.
- **`prepare: ... 'audio' ... axes`** or a similar refusal naming a tensor — TODO 1: prepare checks every spec against the file and says which one does not fit.
- **`create: configuration error: tensor 'state_in': a State input needs a state_source`** — TODO 2: a state input nobody feeds is refused by name, at create.
- **`FAILED: max abs diff 0.158`** with the latency reported — the state is not carried: block 0 matches, every block after it does not. Same failure as a fresh engine per block in step 1. Check `state_source`.
- **`prepare: not supported in this build: contract: a MEASURED budget ...`** — this anira build needs an explicit budget; keep `{"ms": 8.2}`.
- **Many late callbacks** — the machine is busy, or 8.2 ms is below its worst case. Raise the budget; the latency grows by a host block per 11.6 ms.

Compare with [`solution/`](solution/) when you want to.

## Slides

[`slides/14-anira-inference.md`](../../slides/14-anira-inference.md).

## What's next

The model taken apart: encoder and decoder as two handlers, and eight numbers between them you can reach.
