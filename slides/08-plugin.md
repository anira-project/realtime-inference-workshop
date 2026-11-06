<h1>Step 8<br>The plugin</h1>

Everything from steps 1 to 7, where the host can reach it

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

Note:
- Run the standalone: the example plays, move dry/wet, drop in an audio file, switch to the input
- Everything from steps 1 to 7, in a plugin
- Listen first: switch examples, dry/wet, your own audio files

---

<div class="tag">Host</div>

## Three places the host talks to us

| Host call | What happens | Steps |
|---|---|---|
| **`prepareToPlay`** | load the model, size the buffers, start the worker, report the latency | 1,&nbsp;2,&nbsp;3,&nbsp;4,&nbsp;7 |
| **`processBlock`** | audio in and out between threads, dry/wet mix | 6,&nbsp;7 |
| **`releaseResources`** | stop the worker thread | 6 |

Note:
- What did we add? Not much: the host talks to us in three places
- `prepareToPlay`: load the model, size buffers, start the worker thread, report latency
- `processBlock`: callback from step 7, unchanged
- `releaseResources`: stop the worker thread
- Everything else: JUCE boilerplate
- Built step by step, every step from 1 to 7 is in here:
  - `prepareToPlay`: model (1), state (2), ring buffers (3), worst case in the prefill (4), latency (7)
  - `processBlock`: host blocks (3), real-time safe (5), worker hand-off (6), dry/wet (7)
  - `releaseResources`: stop the worker (6)
- Only 44.1 or 48 kHz and 512: our prefill and latency are only right for that block size. Anything else: bypassed, the input goes out unprocessed, with an overlay that says why
- Takeaway: arbitrary buffer sizes need a different design, part 2
- One change: ONNX Runtime is now linked statically: 28 MB + 126 MB model

--- 

<div class="tag">Test</div>

## Try it yourself<br><span class="then">Start the build now, listen in the break.</span>

Play around with the dry/wet parameter. Use headphones please.

```bash
cmake --build --preset release --target step08_plugin_Standalone # for the standalone
cmake --build --preset release --target step08_plugin_VST3       # for the VST3 plugin

#open the standalone app with:
open build/steps/08_juce_plugin/step08_plugin_artefacts/Release/Standalone/*.app
```
<p class="path-note">The VST3 plugin is in <code>build/steps/08_juce_plugin/step08_plugin_artefacts/Release/VST3</code>.</p>

<div class="cards-note corner-note">Only 44.1 or 48 kHz and 512-sample buffers: anything else is bypassed, and the window says why.</div>


Note:
- Commands are on the slide: standalone or VST3
- Start the build now, ready during the break
- Play with dry/wet, try your own sounds, headphones please
- Set your device or DAW to 512 samples and 44.1 or 48 kHz, otherwise it is bypassed and says so
- Sample rate warning: model expects 44.1 kHz
- At 48 kHz it runs but sounds different: set your device to 44.1

---

<div class="tag">Break</div>

## Break<br><span class="then">Afterwards: how anira handles these steps.</span>

<div class="break-mug">{{ICON:coffee}}<div class="break-back" style="margin-top: 150px;">back at 11:35</div></div>

Note:
- Every piece is a place where things can go wrong
- You'd write all of it again for every new model
- After the break: how anira does this for you
- 15 minute break, we start again at 11:35
