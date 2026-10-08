<h1>Step 8<br>The plugin</h1>

Everything from steps 1 to 7, where the host can reach it

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

Note:
    - Run the standalone here. Play through it, move the dry/wet.

---

## Three places the host talks to us

**`prepareToPlay`** — load the model, size the buffers, start the threads

```cpp
setLatencySamples(static_cast<int>(processor.latency_samples()));
```

**`processBlock`** — step 6's callback, unchanged

**`releaseResources`** — stop the threads

Everything else in the plugin is JUCE boilerplate.

---

## Which engine ships

Every step so far used LibTorch. The plugin uses **ONNX Runtime, static**:

| | what ships |
|---|---|
| LibTorch | binary + 318 MB of dylibs to find at load time |
| ONNX Runtime, static | **one 28 MB bundle** |

The switch is one type name — the processor is templated on the engine, and
both engines have the same three methods.

Note:
    - This is step 4's argument arriving: the size difference is not theoretical, it is the bundle you upload.

---

## One engine per channel

The model is **mono and stateful**. Two channels need two states — sharing one
would mix the left channel's history into the right.

So: two engines, two worker threads, two of everything.

For one plugin instance. On one track.

Note:
    - Let that sit. This is the problem the second block opens with.

---

## How you check the latency in a DAW

Dry/Wet at 0, delay compensation on:

> a fully dry instance has to **null** against the original track

Same test as in step 7, now in the host. If it does not null, the number is
wrong, and everything downstream of it is too.

---

## What it took

```
ring buffers          step 2
a benchmark           step 3
an engine choice      step 4
RTSan                 step 5
a worker thread       step 6
two lock-free queues  step 6
a prefill rule        step 7
a latency calculation step 7
a dry path to match   step 7
one engine per channel step 8
```

All of it before a single creative decision.

---

## Break

After it: what anira does with all of this.

<!-- .slide: data-state="no-footer" -->

Note:
    - 30 minutes. Then the architecture talk.
