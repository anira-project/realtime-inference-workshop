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

1. **`prepareToPlay`** — load the model, size the buffers, start the threads

<!-- ```cpp
setLatencySamples(static_cast<int>(processor.latency_samples()));
``` -->

2. **`processBlock`** — callback from step 6, unchanged

3. **`releaseResources`** — stop the threads

Everything else in the plugin is JUCE boilerplate.

Note:
- What did we add? Not much: the host talks to us in three places
- `prepareToPlay`: load the model, size buffers, start worker threads, report latency
- `processBlock`: callback from step 6, unchanged
- `releaseResources`: stop the threads
- Everything else: JUCE boilerplate
- One change: ONNX Runtime instead of LibTorch, statically linked: 28 MB + 126 MB model

**Script:**

"So what did we have to add to turn our processor into a plugin? Not much, actually. The host talks to us in three places. In `prepareToPlay` we load the model, size the buffers, start the worker threads, and tell the host our latency. `processBlock` is our callback from step 6, unchanged. And in `releaseResources` we stop the threads again. Everything else in the plugin is JUCE boilerplate.

One change: every step so far used LibTorch, but the plugin uses ONNX Runtime, linked statically, which is 28 MB of engine, plus the 126 MB model."

---

<div class="tag">Engine</div>

## One plugin, stereo.<br><span class="then">Two of everything, and the host keeps adding more.</span>

<div class="per-channel">
  <b>Left</b><span>engine</span><i>→</i><span>worker thread</span>
  <b>Right</b><span>engine</span><i>→</i><span>worker thread</span>
</div>

<p style="margin-top: var(--space-9);"><strong>10 stereo tracks = 20 engines, 20 worker threads</strong></p>

Note:
- Model is mono and remembers the past
- Left and right are different signals: each needs its own memory
- Two engines, and in our design two worker threads
- That's one plugin on one track
- Ten tracks: twenty engines, twenty worker threads, and the host can add more
- Keep that in mind: part 2 starts there

**Script:**

"One more thing we have to handle in the plugin. Our model is mono, and as we saw, it remembers the past. Left and right are different signals, so each needs its own memory. That means two engines, and in our design, two worker threads. And that's for one plugin on one track. Put it on ten tracks and you have twenty engines and twenty worker threads, and the host can keep adding more. Keep that in mind, because that's where part 2 starts."

---

<div class="tag">Summary</div>

## All of this is infrastructure,<br><span class="then">before any work on the sound itself.</span>

```
ring buffers           step 2
a benchmark            step 3
RTSan                  step 4
a worker, lock-free ring buffers   step 5
a prefill rule         step 6
a latency calculation  step 6
a dry path to match    step 6
an engine choice       step 7
one engine per channel step 8
```

Note:
- Look back at what we built
- Step 1: first C++ implementation
- Step 2: buffer sizes, ring buffers
- Step 3: benchmark
- Step 4: real-time safety, the sanitizer. Basically impossible with the model on the audio thread
- Step 5: worker thread, lock-free ring buffers
- Step 6: latency, align dry and wet
- Step 7: engine options
- Step 8: first neural inference plugin/standalone
- All infrastructure, no work on the sound itself yet

**Script:**

"So let's look back at what we built. In step 1 we managed to do a first implementation of our model in C++. In step 2 we had the problem of a potential issue with the buffer size. The solution was to use Ring buffers. In step 3 we benchmarked our signal processing. In step 4 we talked about realtime safety and how to check it using a sanitizer. We noticed that it was basically impossible to be realtime safe with the model on our audio thread, so we introduced a worker thread with lock-free ring buffers in step 5. Then we talked about latency and how to calculate it. We need to know the latency to align the dry and wet signals properly. Then we looked at different engine options in step 7. And finally, we made our first neural inference plugin/standalone app with one engine in step 8.
All of this is infrastructure and to get a grasp of what is needed for a real-time neural inference plugin. And we haven't done any work on the sound itself yet."

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
- Sample rate warning: model expects 44.1 kHz
- At 48 kHz it runs but sounds different: set your device to 44.1

**Script:**

"If you want to try it yourself, the commands are on the slide, as a standalone app or as a VST3. Start the build now and it will be ready during the break. Play around with the dry/wet parameter and try your own sounds, with headphones please. If the plugin warns you about the sample rate, that's because the model expects 44.1 kHz. At 48 kHz it still runs, but it sounds different, so set your device to 44.1 to hear it as intended."



<!-- <div class="tag">Test</div>

## Check the latency in a DAW<br><span class="then">Fully dry has to cancel out.</span>

1. Duplicate a track, put the plugin on the copy
2. Dry/Wet at **0**, delay compensation **on**
3. Flip the phase of the copy

**Silence:** the reported latency is right.<br>
**Anything else:** this track is out of time with the rest of the session.

(hidden) Note:
    - At 0 the plugin outputs only the dry path, delayed by 6320 samples. The host delays every other track by the latency we report, so if the number is right, the two copies line up sample for sample and cancel.
    - This is the step 6 alignment, checked by the host instead of by us.

**Script:**

"And here's a trick to check our latency in a real DAW. Duplicate a track and put the plugin on the copy. Set dry/wet to zero and turn delay compensation on. Then flip the phase of the copy.
At zero, the plugin outputs only the delayed dry signal. If the latency we report is right, the host delays the original track by exactly the same amount, and the two copies cancel out completely. So you should hear silence. If you hear anything else, our track is out of time with the rest of the session. It's the same alignment we checked in step 6, but this time the host checks it for us."
-->
---

<div class="tag">Break</div>

## Break<br><span class="then">Afterwards: how anira handles these steps.</span>

<div class="break-mug">{{ICON:coffee}}</div>

Note:
- Every piece is a place where things can go wrong
- You'd write all of it again for every new model
- After the break: how anira does this for you
- 15 minute break

**Script:**

"Every one of these pieces is a place where things can go wrong. And you'd have to write all of it again for every new model. After the break, we'll look at how anira does all of this for you. Let's take a 15 minute break, see you then."
