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



<div class="tradeoff ships">
  <div class="tradeoff-name">LibTorch</div>
  <div class="tradeoff-bar weight" style="--v:100%;--i:0"><span>318 MB</span></div>
  <div class="tradeoff-name">ONNX Runtime, static</div>
  <div class="tradeoff-bar weight" style="--v:8.8%;--i:1"><span>28 MB</span></div>
</div>

```cpp
LatencyProcessor<LibTorchEngine>   // steps 1 to 7
LatencyProcessor<OnnxEngine>       // the plugin
```

Note:
    - Every step so far used LibTorch: the binary plus 318 MB of dylibs to find at load time. The plugin uses ONNX Runtime, linked statically: one 28 MB bundle.
    - Switching is one type name: the processor is templated on the engine, and both engines have the same three methods.
    - This is step 4's argument arriving: the size difference is not theoretical, it is the bundle you upload.

---

<div class="tag">Engine</div>

## A mono, stateful model<br><span class="then">Two of everything, for one plugin on one track.</span>


<div class="per-channel">
Mono:
  <b>mono</b><span>state</span><i>→</i><span>engine</span><i>→</i><span>worker thread</span>
</div>

<div class="per-channel">
Stereo:
  <b>Left</b><span>state</span><i>→</i><span>engine</span><i>→</i><span>worker thread</span>
  <b>Right</b><span>state</span><i>→</i><span>engine</span><i>→</i><span>worker thread</span>
</div>

Note:
    - The model is mono and stateful: each channel needs its own state, so each channel needs its own engine.
    - Here, each engine also gets its own worker thread.
    - Two channels → two states → two engines → two worker threads. Keep it in mind: part 2 starts here.

---

<div class="tag">Test</div>

## Build it and listen<br><span class="then">Standalone or VST3.</span>

Build the implementation and play around with the dry/wet parameter

```bash
cmake -S . -B build-juce -G Ninja -DCMAKE_BUILD_TYPE=Release -DWORKSHOP_JUCE=ON
cmake --build build-juce --target step08_plugin_Standalone   # or step08_plugin_VST3
```


--- 

<div class="tag">Test</div>

## Check the latency in a DAW<br><span class="then">Fully dry has to cancel out.</span>

1. Duplicate a track, put the plugin on the copy
2. Dry/Wet at **0**, delay compensation **on**
3. Flip the phase of the copy

**Silence:** the reported latency is right.<br>
**Anything else:** this track is out of time with the rest of the session.

Note:
    - At 0 the plugin outputs only the dry path, delayed by 6320 samples. The host delays every other track by the latency we report, so if the number is right, the two copies line up sample for sample and cancel.
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

<div class="break-mug">{{ICON:coffee}}</div>

Note:
    - 30 minutes. Then the architecture talk.
