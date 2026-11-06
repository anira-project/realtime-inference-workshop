# Step 9: The model with anira

**Goal:** run the part 1 model on anira. Two JSON files say what the model is and how it runs; three calls connect it to the host. Steps 3, 6 and 7 are the library's now.

**Problem it solves:** everything from part 1 was infrastructure, and every piece of it is a place to get it wrong. Here it is declared, not implemented.

## What you have

- `steps/common/assets/models/rave/model_forward.onnx` — the model as in step 1: audio plus a state tensor in, audio plus the new state out.
- [`exercise/model.json`](exercise/model.json) — the model file, every key in place: the tensors are named and their roles given. Every `TODO1` and `TODO2` is a blank: replace it with the value, a number as it is (`2048`, no quotes), a name in quotes (`"audio"`). The comment next to each blank says where the value comes from.
- [`exercise/contract.json`](exercise/contract.json) — how it runs: the budget measured at prepare on this machine (`"budget": "measured"`), warm-up runs until their times are stable (`"warmup": "until_stable"`), the input block on a miss (`"on_miss": "bypass"`). Complete.
- [`common/helpers/host.h`](../common/helpers/host.h) — the fake host from step 6, pacing its callbacks.
- [`common/helpers/support.h`](../common/helpers/support.h) — `report_with_latency`: the step 7 comparison, with the late callbacks left out.
- anira, fetched and built by CMake ([`cmake/anira.cmake`](../../cmake/anira.cmake)), with ONNX Runtime built in. The first configure clones it and builds it once; a couple of minutes.

## What to do

Open [`exercise/model.json`](exercise/model.json) and [`exercise/AniraProcessor.cpp`](exercise/AniraProcessor.cpp).

1. **Declare the tensors** (`model.json`). The sizes: `"time"` of the audio in and out is the model's block, 2048 (step 1). The audio output carries the model's own delay: `"latency": 2224` (step 7).
2. **Declare the state** (`model.json`). Its size, in and out: as many floats as the state of step 2 (63,488). And `state_source`: the output that feeds `state_in`. Is it `"audio"`, `"audio_out"` or `"state_out"`? Try; a wrong one is refused at create, with the reason. Once paired, anira carries the state from call to call: the whole state loop of step 2, with no copy in your code.
3. **Prepare and process** (`AniraProcessor.cpp`). Loading, the pipeline, the handler and prepare are given, all from `anira.hpp`; four blanks, marked `fill out`:
   1. `m_contract.hard_host_blocks(…)`: the smallest block the host may send (any, down to 1), its largest (`max_block_size`) and its sample rate.
   2. `latency`: `handler.get_latency` of output slot 0.
   3. Offline, a bounce: `m_handler->process_wait` over the block, with the timeout `ANIRA_WAIT_FOREVER`: it waits for each result, so nothing is missed.
   4. Live, in the audio callback: `m_handler->process` over the block, in place, so the block is both input and output. It never waits.

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
Measured budget:  12.7 ms per inference, at prepare
Reported latency: 5295 samples (120.1 ms at 44100 Hz)

OK: max abs diff 3.13e-07, within 0.0001 of the reference.
OK: live, the blocks are processed too.
```

Both numbers are this machine's: prepare measures the budget while it warms the model up, and the latency follows from it. Yours differ; a slower machine gets a larger latency, not dropouts. 5295 covers host blocks from 1 to 512, as the contract declares: the host may call with any block up to its largest, so the latency covers the worst one. With every block exactly 512 it would be 4784, the number of step 7.

The check runs twice. First offline, like a bounce: the host calls as fast as it can and every block waits for its result, so nothing is missed and the comparison is exact: the output against the reference, shifted by the reported latency less the model's own 2224, which the reference already contains. Then once live, at the host's pace, with your `process` line: it only has to process the blocks. Live, a block that is not back in time goes out as the miss policy's fill, here the input itself (`"on_miss": "bypass"`).

## If you're stuck

- **`model.json, line 13: TODO1 is still open`** — a blank not filled yet; the line says which.
- **`model.json, line 13: "2048" is text; write the number without quotes`** — a number in quotes is a string in JSON.
- **`inputs[0].axes[2][1]: an extent is a positive integer or "dynamic"`** — a size is not a positive number. The path names it: `inputs[0]` is `audio`, `inputs[1]` is `state_in`, `outputs[…]` the same for the outputs.
- **`prepare: ... 'audio' ... axes`** or a similar refusal naming a tensor — TODO 1: a size is filled but wrong; prepare checks every spec against the file and says which one does not fit.
- **`create: configuration error: tensor 'state_in': a State input needs a state_source`** — TODO 2: a state input nobody feeds is refused by name, at create.
- **`prepare: configuration error: contract: Hard host blocks missing`** — TODO 3.1: the contract does not know the host's blocks yet.
- **`TODO 3: read the latency of output slot 0`** — TODO 3.2.
- **`state_source 'TODO2' names no output tensor`** (or `'audio'`) — TODO 2: no output has that name.
- **`state_source 'audio_out' is a Streamed output`** — TODO 2: a state input is fed from the state output, not from the audio.
- **`TODO 1: audio_out declares no latency`** — `"latency"` in `model.json` is 0.
- **`TODO 3: offline, the output is the input`** — TODO 3.3: the block goes out untouched; `process_wait` is still commented out.
- **`TODO 3: offline, N blocks were missed`** — TODO 3.3: the offline call does not wait; give it `ANIRA_WAIT_FOREVER`.
- **`TODO 3: live, the output is the input`** — TODO 3.4: `process` is still commented out.
- **`FAILED: max abs diff 0.158`** with the latency reported — the state is not carried: block 0 matches, every block after it does not. Same failure as a fresh engine per block in step 1. Check `state_source`.
- **Many late callbacks** — the machine was busier during the stream than while prepare measured. Close what else runs, or give the budget headroom by hand, `"budget": {"ms": 20}`; the latency grows with it.

Compare with [`solution/`](solution/) when you want to.

## Slides

[`slides/15-anira-inference.md`](../../slides/15-anira-inference.md).

## What's next

The model taken apart: encoder and decoder as two handlers, and eight numbers between them you can reach.
