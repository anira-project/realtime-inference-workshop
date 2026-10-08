# Step 8: The plugin

**No exercise here** — everything in this plugin comes from steps 1 to 7. What is new is only where the host touches it.

**Goal:** hear it. And see that the real-time work is already done.

## Building it

JUCE is cloned and built, which takes a few minutes, so it is off by default:

```bash
cmake -S . -B build-juce -G Ninja -DCMAKE_BUILD_TYPE=Release -DWORKSHOP_JUCE=ON
cmake --build build-juce --target step08_plugin_Standalone   # or step08_plugin_VST3
open "build-juce/steps/08_juce_plugin/step08_plugin_artefacts/Release/Standalone/Workshop Inference.app"
```

The VST3 passes `pluginval --strictness-level 5`.

## The three places the host talks to us

**`prepareToPlay`** — everything that allocates: load the model, size the ring buffers, start the worker threads. Then the number from step 7:

```cpp
setLatencySamples(static_cast<int>(m_channels.front()->latency_samples()));
```

That one line is what lets the host line our track up with every other track.

**`processBlock`** — the audio callback from step 6, unchanged: ring buffer in, `try_enqueue`, `try_dequeue`, mix, out. No engine, no allocation, no lock.

**`releaseResources`** — stop the worker threads.

## What is per channel

One engine and one processor per channel. The model is mono and stateful, so two channels need two states — sharing one would mix the left channel's history into the right. That also doubles the memory and the worker threads, which is exactly the problem the second block of the workshop is about.

## What it sounds like

The `Dry/Wet` parameter is the mix from step 7. At 0 you hear the input, delayed by 4096 samples and nothing else — which is also how you check the latency is right: in a DAW, with delay compensation on, a fully dry instance has to null against the original track.

## Which engine, and why it matters here

The plugin runs **ONNX Runtime, linked statically** — not LibTorch, which every
step before it used. The reason is step 4, now with a bundle around it:

| | what ships |
|---|---|
| LibTorch | the binary plus 318 MB of dylibs that have to be found at load time |
| ONNX Runtime, static | **one 28 MB bundle**, nothing to find |

LibTorch has no static build in the backends release, so the choice is made for
you. The engine sits behind `construct / process / reset`, so the switch is one
type name in `PluginProcessor.h` — the processor itself is templated on it.

## Known rough edges, on purpose

- **The model path is absolute**, baked in at compile time, and `forward.onnx.data` has to stay next to `forward.onnx`. A real plugin ships both inside the bundle.
- **The sample rate is fixed at 48 kHz** by the export. At another rate the plugin says so and still runs; it is a different instrument then, not a broken one.

## Slides

[`slides/08-plugin.md`](../../slides/08-plugin.md).

## What's next

This is the end of the first block: a correct, real-time safe, latency-reporting plugin, built from scratch. Count what it took — ring buffers, a worker thread, two queues, a prefill rule, a latency calculation, a dry path, one engine per channel.

After the break: what anira does with all of this, and what it does better.
