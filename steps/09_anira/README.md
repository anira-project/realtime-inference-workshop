# Step 9: The model with anira

**Goal:** run the part 1 model on anira. Two JSON files say what the model is and how it runs; three calls connect it to the host. Steps 3, 6 and 7 are the library's now.

**Problem it solves:** everything from part 1 was infrastructure, and every piece of it is a place to get it wrong. Here it is declared, not implemented.

## What you have

- `steps/common/assets/models/rave/model_forward.onnx` — the model as in step 1: audio plus a state tensor in, audio plus the new state out.
- [`exercise/model.json`](exercise/model.json) — the model file, every key in place: the tensors are named and their roles given. Every `?` is a blank: replace it with the value, a number as it is (`2048`, no quotes), a name in its quotes (`"audio_in"`). The comment next to each blank says which TODO it belongs to and where the value comes from.
- [`exercise/contract.json`](exercise/contract.json) — how it runs: the budget measured at prepare on this machine (`"budget": "measured"`), warm-up runs until their times are stable (`"warmup": "until_stable"`), the input block on a miss (`"on_miss": "bypass"`). Complete.
- [`common/helpers/support.h`](../common/helpers/support.h) — `report_with_latency`: the step 7 comparison, with the late callbacks left out.
- anira, fetched and built by CMake ([`cmake/anira.cmake`](../../cmake/anira.cmake)), with ONNX Runtime built in. The first configure clones it and builds it once; a couple of minutes.

## What to do

Open [`exercise/model.json`](exercise/model.json) and [`exercise/AniraProcessor.cpp`](exercise/AniraProcessor.cpp).

1. **Declare the tensors** (`model.json`). The sizes: `"time"` of the audio in and out is the model's block, 2048 (step 1). The audio output carries the model's own delay: `"latency": 2224` (step 7).
2. **Declare the state** (`model.json`). Its size, in and out: as many floats as the state of step 2 (63,488). And `state_source`: the output that feeds `state_in`. Which one is it? Try; a wrong one is refused at create, with the reason. Once paired, anira carries the state from call to call: the whole state loop of step 2, with no copy in your code.
3. **Prepare and process** (`AniraProcessor.cpp`). Loading, the pipeline, the handler and prepare are given, all from `anira.hpp`; four blanks, marked `fill out`:
   1. `m_contract.hard_host_blocks(…)`: the smallest block the host may send (any, down to 1), its largest (`max_block_size`) and its sample rate.
   2. `latency`: `handler.get_latency` of output slot 0.
   3. Offline, a bounce: `m_handler->process_wait` over the block, with the timeout `ANIRA_WAIT_FOREVER`: it waits for each result, so nothing is missed.
   4. Live, in the audio callback: `m_handler->process` over the block, in place, so the block is both input and output. It never waits.

Build and run from the repository root:

```bash
cmake --build --preset release --target step09_exercise_check
./build/release/bin/step09_exercise_check
```

Once it passes, the same `AniraProcessor` runs as a plugin:

```bash
cmake --build --preset release --target step09_exercise_Standalone   # or step09_exercise_VST3
```

The result is in `build/release/steps/09_anira/step09_exercise_artefacts/Release/`.

In the window:

- **report**, top right, shows what `prepare` measured on your machine: the budget, the measured and the median inference time, the warm-up, and the load.
- The load is the inference time over the audio one inference produces. Its verdict: comfortable below 75 %, marginal below 100 %, else not real-time.
- A model that is not real-time does not play live; the overlay says so. **Try nonetheless** plays it all the same, with dropouts and clicks. A bounce always processes: it waits for every result.
- A sample rate other than 44.1 kHz asks first too. The model is trained at 44.1 kHz and would sound higher or lower, and the plugin has no resampler. **Process anyway** plays it as it is.
- If both apply, the window asks for one, then for the other.

## Expected output

```
anira's report, model (at prepare, on this machine):
  budget 12.9 ms   measured 12.9 ms   median 12.8 ms
  warm-up 4 inferences, until stable
  load 28 % (real-time factor 0.28 over 1 inference): comfortable

  reported latency             5295 samples (120.1 ms at 44100 Hz)
  PASS  the output matches the reference: max abs diff 3.13e-07, tolerance 0.0001
  PASS  live, at the host's pace, the blocks are processed too

==============================================================
  STEP 9 SOLVED
==============================================================
```

The report and the latency are this machine's. prepare measures the budget while it warms the model up, and the latency follows from it. Yours differ: a slower machine gets a larger latency, not dropouts.

The load is the inference time over the audio one inference produces. "Over 1 inference" because the model carries its state: its inferences cannot overlap, they run one after another.

5295 covers host blocks from 1 to 512, as the contract declares. The host may call with any block up to its largest, so the latency covers the worst one. With every block exactly 512 it would be 4784, the number of step 7.

The check runs twice. First offline, like a bounce: the host calls as fast as it can, with blocks of changing size up to 512, and every block waits for its result. Nothing is missed, so the comparison is exact. The output is compared with the reference, shifted by the reported latency less the model's own 2224, which the reference already contains. Then once live, at the host's pace, with your `process` line. Live, a block that is not back in time goes out as the miss policy's fill, here the input itself (`"on_miss": "bypass"`).

## If you're stuck

`AniraProcessor.cpp` builds once every `?` in it is filled in. The `?` in `model.json` are found when the check runs. By TODO:

- **`model.json, line 13: ? is still open`** (or `"?" is still open`) — a blank not filled yet; the line says which.
- **`model.json, line 13: "2048" is text; write the number without quotes`** — a number in quotes is a string in JSON.
- **`inputs[0].axes[2][1]: an extent is a positive integer or "dynamic"`** — a size is not a positive number. The path names it: `inputs[0]` is `audio_in`, `inputs[1]` is `state_in`, `outputs[…]` the same for the outputs.
- **`onnxruntime: input tensor 'audio' bound to the model's 'audio': the model's tensor is [1, 1, 2048], the spec's engine dims are [1, 1, 1024] (axis 2: 2048 against 1024)`** — TODO 1 (or 2, for `state_in`): a size is filled but wrong; prepare checks every tensor against the model file and names the one that does not fit, with both shapes.
- **`TODO 1: audio_out declares a latency of 0 samples; the model's own delay is 2224`**, after a mismatch — the output lines up with the reference, but at another shift: `"latency"` in `model.json` is wrong (here 0). The reported latency moves with it, the output does not.
- **`tensor 'state_in': a State input needs a state_source: ...`** — TODO 2: a state input nobody feeds is refused by name, at create.
- **`state_source 'audio' names no output tensor`** — TODO 2: no output has that name.
- **`state_source 'audio_out' is a Streamed output`** — TODO 2: a state input is fed from the state output, not from the audio.
- **`the output does not match the reference: max abs diff 0.158`** with the latency reported — the state is not carried: block 0 matches, every block after it does not. Same failure as a fresh engine per block in step 1. Check `state_source`.
- **`prepare: configuration error: contract: Hard host blocks missing`** — TODO 3.1: the contract does not know the host's blocks yet.
- **`a latency of 46147 samples leaves nothing of 16384 to compare`** — TODO 3.1: the arguments of `hard_host_blocks` are in the wrong order; the sample rate went in as a block size.
- **`TODO 3: offline, N blocks were missed once they changed size`** — TODO 3.1: the contract does not cover every block the host may send; its smallest is 1.
- **`TODO 3: read the latency of output slot 0`** — TODO 3.2.
- **`TODO 3: offline, the output is the input`** — TODO 3.3: the block goes out untouched; the offline call does not process it.
- **`TODO 3: offline, N blocks were missed`** — TODO 3.3: the offline call does not wait; give it `ANIRA_WAIT_FOREVER`.
- **`TODO 3: live, the output is the input`** — TODO 3.4: the live call does not process the block.
- **Many late callbacks** — the machine was busier during the stream than while prepare measured. Close what else runs, or give the budget headroom by hand, `"budget": {"ms": 20}`; the latency grows with it.

Compare with [`solution/`](solution/) when you want to.

## Slides

[`slides/15-anira-inference.md`](../../slides/15-anira-inference.md).

## What's next

The model as an instrument: a camera and hand tracking drive the decoder.
