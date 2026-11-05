# Step 8: The plugin

**No exercise here** — everything in this plugin comes from steps 1 to 7. What is new is only where the host touches it.

**Goal:** hear it. And see that the real-time work is already done.

## Building it

The first build of JUCE takes a few minutes:

```bash
cmake --preset release
cmake --build --preset release --target step08_plugin_Standalone   # or step08_plugin_VST3
open "build/steps/08_juce_plugin/step08_plugin_artefacts/Release/Standalone/Workshop Inference.app"
```

The VST3 passes `pluginval --strictness-level 5`.

## The processor is step 7's

[`solution/ProcessorExample.h`](solution/ProcessorExample.h) is step 7's `ProcessorExample`, filled in, one-to-one: the same constants, the same `prepare`, `process_block` and `process_model`, with its `WorkerThread` inside. [`solution/PluginProcessor`](solution/PluginProcessor.cpp) owns the engine and that one processor, as step 7's `main` does, and writes only the places where the host talks to us.

Everything else is the plugin shell in [`common/plugin/`](../common/plugin/), shared with step 9: `PluginBase` (buses, the dry/wet parameter, the host boilerplate), the window in `ui/`, and the source player in `dsp/`. Your number from step 4 comes from `k_max_inference_ms` in [`common/inference_time.h`](../common/inference_time.h), as in steps 6 and 7.

## The three places the host talks to us

**`prepareToPlay`**: everything that allocates. Load the model once, `prepare` the processor, which sizes the ring buffers, warms up and starts the worker thread. Then the number from step 7:

```cpp
setLatencySamples(static_cast<int>(m_processor->latency_samples()));
```

That one line is what lets the host line our track up with every other track: 1536 + the rounded max inference time + 2224, 4784 samples with `k_max_inference_ms` from 11.6 to 23.2 ms.

**`processBlock`**: the mono mix-down, then step 7's `process_block`, unchanged. No engine, no allocation, no lock.

**`releaseResources`**: stop the worker thread.

## Only 44.1 or 48 kHz, only 512 samples

The prefill and the latency from steps 6 and 7 are only right for a host block of 512, and the model is trained at 44.1 kHz (at 48 kHz it runs, a little higher). For anything else the plugin is **bypassed**: the input goes out unprocessed, the latency is 0, and the window shows an overlay that says why. The same happens to a block of another size in the middle of playback. Arbitrary rates and buffer sizes need a different design; that is part 2.

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
- **44.1 or 48 kHz and 512 samples only.** Anything else is bypassed, and the window says so: set the audio device or the DAW to match.
- **The worker busy-waits.** It checks for a new block without sleeping, which keeps one core busy.

## Slides

[`slides/08-plugin.md`](../../slides/08-plugin.md).

## What's next

This is the end of the first block: a correct, real-time safe, latency-reporting plugin, built from scratch. Count what it took — ring buffers, a worker thread, a prefill rule, a latency calculation, a dry path, one engine mixed down to mono.

After the break: what anira does with all of this, and what it does better.
