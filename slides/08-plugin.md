<h1>Step 8<br>The plugin</h1>

<!-- Everything from steps 1 to 7 in a JUCE plugin -->

<!-- .slide: data-state="no-header" -->
<!-- kind: demo -->

Note:
- Next: all of this in a JUCE plugin, where the host asks for exactly these numbers
- For this we prepared a JUCE plug in that uses everything from steps 1 to 7, in a plugin

---

<div class="tag">Host</div>

## Everything from steps 1 to 7 in a JUCE plugin

| Host call | What happens | Steps |
|---|---|---|
| **constructor** | load the model, build the processor | 1,&nbsp;7 |
| **`prepare`** ← `prepareToPlay` | size the buffers, start the worker, return the latency | 2,&nbsp;3,&nbsp;4,&nbsp;7 |
| **`process`** ← `processBlock` | one mono block: in and out between threads, dry/wet mix | 6,&nbsp;7 |

<div class="cards-note corner-note">And <code>release</code> stops the worker thread again. Bypass, mono and stereo: <code>PluginBase</code>.</div>

Note:
- What did we add? Not much: the host talks to us in three places
- Constructor: load the model once, build the processor; the host may construct the plugin just to scan it
- `prepare`, from `prepareToPlay`: size buffers, start the worker thread, return the latency; the host may call it again at any time
- `process`, from `processBlock`: callback from step 7, unchanged, on the input mixed down to mono
- `release`, from `releaseResources`: stop the worker thread
- PluginBase does the rest: the bypass, mono to stereo, telling the host the latency
- The same three places come back with anira: create, prepare, process
- Everything else: JUCE boilerplate
- Only 44.1 kHz and 512: our prefill and latency are only right for that block size, and the model and the step 4 number are 44.1 kHz ones. At 48 kHz the window asks first, Process anyway plays it higher; a resampler does not fit our fixed 512 and 2048 blocks. Anything else: bypassed, the player stops and the input goes out unprocessed, with an overlay that says why; the player starts over once the settings fit
- Takeaway: arbitrary buffer sizes need a different design, part 2
- One change: ONNX Runtime is handled a bit differently

--- 

<div class="tag">Test</div>

## Try it yourself<br><span class="then">Start the build now, listen in the break.</span>

Play around with the dry/wet parameter. Use headphones please.

```bash
cmake --build --preset release --target step08_plugin_Standalone # for the standalone
cmake --build --preset release --target step08_plugin_VST3       # for the VST3 plugin

#open the standalone app with:
open build/release/steps/08_juce_plugin/step08_plugin_artefacts/Release/Standalone/*.app
```
<p class="path-note">The VST3 plugin is in <code>build/release/steps/08_juce_plugin/step08_plugin_artefacts/Release/VST3</code>.</p>

<div class="cards-note corner-note">Only 44.1 kHz and 512-sample buffers: 48 kHz asks first, anything else is bypassed, and the window says why.</div>


Note:
- Commands are on the slide: standalone or VST3
- Start the build now, ready during the break
- Play with dry/wet, try your own sounds, headphones please
- Set your device or DAW to 512 samples and 44.1 kHz; 48 kHz asks first, anything else is bypassed and says so
- Sample rate warning: model expects 44.1 kHz
- At 48 kHz it runs but sounds different: set your device to 44.1

---

<div class="tag">Break</div>

## Break<br><span class="then">Afterwards: how anira handles these steps.</span>

<div class="break-mug">{{ICON:coffee}}<div class="break-back" style="margin-top: 150px;">back at 11:40</div></div>

Note:
- Every piece is a place where things can go wrong
- You'd write all of it again for every new model
- After the break: how anira does this for you
- 15 minute break, we start again at 11:40
- Click the timer to start the countdown; a tone with one minute left, three when time is up
