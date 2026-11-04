<h1>Step 8<br>The plugin</h1>

Everything from steps 1 to 7, where the host can reach it

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

Note:
- Run the standalone: click an example tile, move dry/wet, drop in an audio file
- Everything from steps 1 to 7, in a plugin
- Listen first: switch examples, dry/wet, your own audio files

**Script:**

*Run the standalone. Click one of the three example tiles, move dry/wet. Drop an audio file on a tile to replace it.*
"Here it is: everything from steps 1 to 7, in a plugin. Let's listen to it first. You can switch between the examples, move the dry/wet, and drop in your own audio files."

---

<div class="tag">Host</div>

## Three places the host talks to us

1. **`prepareToPlay`** — load the model, size the buffers, start the threads, report the latency

<!-- ```cpp
setLatencySamples(static_cast<int>(LatencyProcessor<OnnxEngine>::latency_samples()));
``` -->

2. **`processBlock`** — callback from step 7, unchanged

3. **`releaseResources`** — stop the threads

Only 512-sample buffers: any other size is silence, and the window says why.

Note:
- What did we add? Not much: the host talks to us in three places
- `prepareToPlay`: load the model, size buffers, start worker threads, report latency
- `processBlock`: callback from step 7, unchanged
- `releaseResources`: stop the threads
- Everything else: JUCE boilerplate
- Only 512: our prefill and latency are only right for that block size. Other sizes: silence and a message
- Takeaway: arbitrary buffer sizes need a different design, part 2
- One change: ONNX Runtime instead of LibTorch, statically linked: 28 MB + 126 MB model

**Script:**

"So what did we have to add to turn our processor into a plugin? Not much, actually. The host talks to us in three places. In `prepareToPlay` we load the model, size the buffers, start the worker threads, and tell the host our latency. `processBlock` is our callback from step 7, unchanged. And in `releaseResources` we stop the threads again. Everything else in the plugin is JUCE boilerplate.

There's one limit, and it's on purpose: our prefill and our latency are only right for a buffer of 512 samples. If the host asks for anything else, the plugin stays silent and tells you why. Supporting any buffer size needs a different design, and that's what part 2 is about.

One change: every step so far used LibTorch, but the plugin uses ONNX Runtime, linked statically, which is 28 MB of engine, plus the 126 MB model."

--- 

<div class="tag">Test</div>

## Try it yourself<br><span class="then">Start the build now, listen in the break.</span>

Standalone or VST3. Play around with the dry/wet parameter, with headphones please.

```bash
cmake -S . -B build-juce -DCMAKE_BUILD_TYPE=Release -DWORKSHOP_JUCE=ON
cmake --build build-juce --config Release --parallel --target step08_plugin_Standalone
```

Or `step08_plugin_VST3`. The result is in `build-juce/steps/08_juce_plugin/step08_plugin_artefacts/Release/`.

Note:
- Commands are on the slide: standalone or VST3
- Start the build now, ready during the break
- Play with dry/wet, try your own sounds, headphones please
- Buffer size: set your device or DAW to 512 samples, otherwise it stays silent and says so
- Sample rate warning: model expects 44.1 kHz
- At 48 kHz it runs but sounds different: set your device to 44.1

**Script:**

"If you want to try it yourself, the commands are on the slide, as a standalone app or as a VST3. Start the build now and it will be ready during the break. Play around with the dry/wet parameter and try your own sounds, with headphones please. Set your audio device or DAW to a buffer of 512 samples, otherwise the plugin stays silent and tells you so. And if it warns you about the sample rate, that's because the model expects 44.1 kHz. At 48 kHz it still runs, but it sounds different, so set your device to 44.1 to hear it as intended."

---

<div class="tag">Break</div>

## Break<br><span class="then">Afterwards: how anira handles these steps.</span>

<div class="break-mug">{{ICON:coffee}}<div class="break-back" style="margin-top: 150px;">back at 11:35</div></div>

Note:
- Every piece is a place where things can go wrong
- You'd write all of it again for every new model
- After the break: how anira does this for you
- 15 minute break, we start again at 11:35

**Script:**

"Every one of these pieces is a place where things can go wrong. And you'd have to write all of it again for every new model. After the break, we'll look at how anira does all of this for you. Let's take a 15 minute break. We start again at 11:35, see you then."
