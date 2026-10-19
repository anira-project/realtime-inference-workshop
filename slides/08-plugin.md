<h1>Step 8<br>The plugin</h1>

Everything from steps 1 to 7, where the host can reach it

<!-- .slide: data-state="no-header" -->
<!-- kind: demo, exercise -->

Note:
    - Run the standalone here. Click one of the three example tiles, move the dry/wet. Dropping an audio file on a tile replaces it.

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

---

<div class="tag">Engine</div>

## ONNX Runtime<br><span class="then">One 28 MB bundle.</span>



| | what ships |
|---|---|
| LibTorch | binary + 318 MB of dylibs to find at load time |
| ONNX Runtime, static | **one 28 MB bundle** |

Every step so far used LibTorch. This plugin uses **ONNX Runtime, static**.

We only need to change one type name — the processor is templated on the engine, and
both engines have the same three methods.

Note:
    - This is step 4's argument arriving: the size difference is not theoretical, it is the bundle you upload.

---

<div class="tag">Engine</div>

## A mono, stateful model

Each channel needs its own state, so each channel needs its own engine.

Here, each engine also gets its own worker thread.

| Two channels → two states → two engines → two worker threads|
|---|

Two of everything. For one plugin, on one track.

---

<div class="tag">Test</div>

## Build it and listen<br><span class="then">Standalone or VST3.</span>

Build the implementation and play around with the dry/wet parameter

```bash
cmake -S . -B build-juce -G Ninja -DCMAKE_BUILD_TYPE=Release -DWORKSHOP_JUCE=ON
cmake --build build-juce --target step08_plugin_Standalone   # or step08_plugin_VST3


--- 

<div class="tag">Test</div>

## Check the latency in a DAW<br><span class="then">Fully dry has to cancel out.</span>

1. Duplicate a track, put the plugin on the copy
2. Dry/Wet at **0**, delay compensation **on**
3. Flip the phase of the copy

**Silence:** the reported latency is right.<br>
**Anything else:** this track is out of time with the rest of the session.

Note:
    - At 0 the plugin outputs only the dry path, delayed by 4096 samples. The host delays every other track by the latency we report, so if the number is right, the two copies line up sample for sample and cancel.
    - This is the step 7 alignment, checked by the host instead of by us.

---



<div class="tag">Summary</div>

## All of this is infrastructure,<br><span class="then">before any work on the sound itself.</span>

```
ring buffers           step 2
a benchmark            step 3
an engine choice       step 4
RTSan                  step 5
a worker, two queues   step 6
a prefill rule         step 7
a latency calculation  step 7
a dry path to match    step 7
one engine per channel step 8
```

---

<div class="tag">Break</div>

## Break<br><span class="then">Afterwards: how anira handles these steps.</span>

<div class="statement-points">
  <div>Ten pieces of infrastructure, by hand</div>
  <div>Every one of them a place to get it wrong</div>
  <div>anira does them for you</div>
</div>


Note:
    - 30 minutes. Then the architecture talk.
