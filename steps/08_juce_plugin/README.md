# Step 8: The plugin

**No exercise here** — everything in this plugin comes from steps 1 to 7. What is new is only where the host touches it.

**Goal:** hear it. And see that the real-time work is already done.

## Building it

The first build of JUCE takes a few minutes:

```bash
cmake --preset release
cmake --build --preset release --target step08_plugin_Standalone   # or step08_plugin_VST3
open "build/release/steps/08_juce_plugin/step08_plugin_artefacts/Release/Standalone/Workshop Inference.app"
```

The VST3 passes `pluginval --strictness-level 5`.

## The processor is step 7's

[`solution/ProcessorExample.h`](solution/ProcessorExample.h) is step 7's `ProcessorExample`, filled in, one-to-one: the same constants, the same `prepare`, `process_block` and `process_model`, with its `WorkerThread` inside. [`solution/PluginProcessor`](solution/PluginProcessor.cpp) owns the engine and that one processor, as step 7's `main` does, and writes only the places where the host talks to us.

Everything else is the plugin shell in [`common/plugin/`](../common/plugin/), shared with steps 9 and 10: `PluginBase` (buses, the dry/wet parameter, the bypass, the mono mix-down, the host boilerplate), the window in `ui/`, and the source player in `dsp/`. `PluginBase` takes the host's `prepareToPlay`, `processBlock` and `releaseResources` and calls the step's `prepare`, `process` and `release`. Your number from step 4 comes from `k_max_inference_ms` in [`common/inference_time.h`](../common/inference_time.h), as in steps 6 and 7.

## The three places the host talks to us

**The constructor** creates: the engine loads the model, the processor holds on to it, once, as in step 7's `main`. Nothing here depends on the host's settings. If the model does not load, the window says so.

**`prepare`**, from the host's `prepareToPlay`: everything that depends on the host's settings, and may run again whenever they change. `prepare` the processor, which sizes the ring buffers and starts the worker thread. Then it returns the number from step 7:

```cpp
return m_processor.latency_samples();
```

`PluginBase` hands it to the host (`setLatencySamples`), so the host lines our track up with every other track: 1536 + the worker's time + 2224, 4784 samples with `k_max_inference_ms` from 11.6 to 23.2 ms.

**`process`**, from the host's `processBlock`: step 7's `process_block`, unchanged, on the input mixed down to mono; the result goes out on every channel. No engine, no allocation, no lock.

And **`release`**, from `releaseResources`, stops the worker thread again.

## Only 44.1 kHz, only 512 samples

The prefill and the latency from steps 6 and 7 are only right for a host block of 512, and the model is trained at 44.1 kHz, as is your step 4 number. At **48 kHz** the window asks first: the model would sound higher and the worker's time is off, and a resampler does not fit our fixed 512 and 2048 blocks. **Process anyway** plays it as it is; until then the input goes out dry, as late as the reported latency, so the track stays in line with the others. For anything else the plugin is **bypassed**: the example or your file stops, the input goes out unprocessed, the latency is 0, and the window shows an overlay that says why. Once the settings fit again, the player starts over from the beginning. Arbitrary rates and buffer sizes need a different design; that is part 2.

## One engine, mono

The model is mono and stateful, so the plugin runs it once: the input channels are mixed down to one signal, that signal goes through one engine and one processor, and the result goes out on every channel. Running a copy per channel would need a second state, a second engine and a second worker thread for every channel. Even so, every instance of the plugin loads the model again and runs its own worker, which is exactly the problem the second block of the workshop is about.

## What it sounds like

The window has three sources:

- **Example**, selected at the start: [`common/assets/audio/example_1.wav`](../common/assets/audio/), compiled into the plugin, in a loop. The standalone needs no microphone and no DAW.
- **Your file**: **load** under the tile, or drop an audio file on the window. It plays in a loop; load another one at any time.
- **Input**: passed through, the host's track, or in the standalone the microphone (unmute it under *Options*).

Files play at their own sample rate; the model expects 44.1 kHz.

The `Dry/Wet` parameter is the mix from step 7. At 0 you hear the input, delayed by the reported latency and nothing else — which is also how you check the latency is right: in a DAW, with delay compensation on, a fully dry instance has to null against the original track.

## One bundle

The plugin links **ONNX Runtime statically**, where the steps before it use it as a shared library. The bundle carries the engine inside it and needs no library next to it: copy the VST3 wherever you like.

## Known rough edges, on purpose

- **The model path is absolute**, baked in at compile time. A real plugin ships `model_forward.onnx` inside the bundle.
- **44.1 kHz and 512 samples only.** 48 kHz asks first (**Process anyway**), anything else is bypassed, and the window says so: set the audio device or the DAW to match.
- **The worker busy-waits.** It spins on `try_pop` and `try_push` without sleeping, which keeps one core busy.

## Slides

[`slides/08-plugin.md`](../../slides/08-plugin.md).

## What's next

This is the end of the first block: a correct, real-time safe, latency-reporting plugin, built from scratch. Count what it took — ring buffers, a worker thread, a prefill rule, a latency calculation, a dry path, one engine mixed down to mono.

After the break: what anira does with all of this, and what it does better.
