<h1>Step 2<br>Stateless and stateful</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- To explain these silences at the beginning of each block we need to talk about the concept of state -> stateless and stateful models

---

<div class="tag">Concept</div>

## Stateless vs. stateful<br><span class="then">Some models read the past at every call, others remember.</span>

<div class="state-compare">
  <div class="state-panel">
    <div class="card-title">Stateless</div>
    <svg class="state-svg stateless" viewBox="0 0 860 380" aria-hidden="true">
  <g class="sv-window">
    <rect class="sv-fill past" x="20.0" y="30" width="273.3" height="90"/>
    <rect class="sv-fill now" x="293.3" y="30" width="136.7" height="90"/>
    <path class="sv-line" d="M20.0 134 v12 H430.0 v-12"/>
    <path class="sv-line" d="M225.0 146 V200"/><path class="sv-head" d="M218.0 192 l7 10 l7 -10z"/>
    <rect class="sv-model" x="145.0" y="206" width="160" height="60" rx="8"/>
    <text x="225.0" y="244" class="sv-label strong">model</text>
  </g>
  <rect class="sv-block" x="20.0" y="30" width="136.7" height="90"/><rect class="sv-block" x="156.7" y="30" width="136.7" height="90"/><rect class="sv-block" x="293.3" y="30" width="136.7" height="90"/><rect class="sv-block" x="430.0" y="30" width="136.7" height="90"/><rect class="sv-block" x="566.7" y="30" width="136.7" height="90"/><rect class="sv-block" x="703.3" y="30" width="136.7" height="90"/>
  <path class="sv-wave" d="M20 75.0 L24 80.8 L28 86.4 L32 90.9 L36 93.5 L40 93.6 L44 91.0 L48 85.9 L52 78.9 L56 70.9 L60 63.0 L64 56.3 L68 51.9 L72 50.5 L76 52.5 L80 57.8 L84 65.6 L88 75.0 L92 84.7 L96 93.3 L100 99.5 L104 102.5 L108 101.7 L112 97.3 L116 89.7 L120 80.1 L124 69.9 L128 60.3 L132 52.8 L136 48.4 L140 47.7 L144 50.7 L148 56.9 L152 65.4 L156 75.0 L160 84.2 L164 91.9 L168 97.0 L172 98.9 L176 97.5 L180 93.2 L184 86.7 L188 78.9 L192 71.2 L196 64.5 L200 59.6 L204 57.2 L208 57.3 L212 59.8 L216 64.1 L220 69.4 L224 75.0 L228 80.0 L232 83.8 L236 86.0 L240 86.5 L244 85.4 L248 83.0 L252 79.9 L256 76.6 L260 73.5 L264 71.1 L268 69.6 L272 69.0 L276 69.3 L280 70.3 L284 71.8 L288 73.4 L292 75.0 L296 76.3 L300 77.3 L304 77.9 L308 78.0 L312 77.8 L316 77.3 L320 76.5 L324 75.5 L328 74.5 L332 73.4 L336 72.4 L340 71.7 L344 71.3 L348 71.4 L352 72.1 L356 73.3 L360 75.0 L364 77.1 L368 79.2 L372 81.1 L376 82.3 L380 82.7 L384 81.9 L388 79.9 L392 76.8 L396 73.0 L400 68.9 L404 65.2 L408 62.5 L412 61.2 L416 61.9 L420 64.6 L424 69.2 L428 75.0 L432 81.4 L436 87.5 L440 92.3 L444 95.0 L448 95.0 L452 92.1 L456 86.7 L460 79.2 L464 70.7 L468 62.4 L472 55.4 L476 50.9 L480 49.5 L484 51.7 L488 57.2 L492 65.3 L496 75.0 L500 84.9 L504 93.6 L508 99.9 L512 102.8 L516 101.9 L520 97.3 L524 89.7 L528 80.1 L532 69.9 L536 60.5 L540 53.1 L544 48.9 L548 48.3 L552 51.3 L556 57.5 L560 65.8 L564 75.0 L568 83.9 L572 91.2 L576 96.0 L580 97.7 L584 96.3 L588 92.1 L592 85.9 L596 78.7 L600 71.5 L604 65.2 L608 60.8 L612 58.7 L616 58.9 L620 61.2 L624 65.2 L628 70.0 L632 75.0 L636 79.4 L640 82.8 L644 84.6 L648 85.0 L652 84.0 L656 81.9 L660 79.2 L664 76.4 L668 73.8 L672 71.7 L676 70.5 L680 70.0 L684 70.3 L688 71.1 L692 72.3 L696 73.7 L700 75.0 L704 76.2 L708 77.0 L712 77.6 L716 77.8 L720 77.7 L724 77.3 L728 76.5 L732 75.6 L736 74.4 L740 73.2 L744 72.1 L748 71.2 L752 70.7 L756 70.7 L760 71.5 L764 72.9 L768 75.0 L772 77.4 L776 80.0 L780 82.2 L784 83.6 L788 84.0 L792 83.0 L796 80.7 L800 77.1 L804 72.7 L808 68.1 L812 64.0 L816 60.9 L820 59.7 L824 60.5 L828 63.6 L832 68.6 L836 75.0 L840 82.0"/>
  <rect class="sv-legend past" x="210" y="316" width="18" height="18" rx="3"/><text x="236" y="331" class="sv-label left">past, read again</text>
  <rect class="sv-legend now" x="440" y="316" width="18" height="18" rx="3"/><text x="466" y="331" class="sv-label left">current block</text>
</svg>
  </div>
  <div class="state-panel">
    <div class="card-title">Stateful</div>
    <svg class="state-svg stateful" viewBox="0 0 860 380" aria-hidden="true">
  <g class="sv-call c0">
    <rect class="sv-fill now" x="293.3" y="30" width="136.7" height="90"/>
    <path class="sv-line" d="M361.7 128 V200"/><path class="sv-head" d="M354.7 192 l7 10 l7 -10z"/>
    <rect class="sv-model" x="319.7" y="206" width="84" height="60" rx="8"/>
    <text x="361.7" y="244" class="sv-label strong">call 1</text>
  </g><g class="sv-state s0">
    <path class="sv-line state" d="M407.7 236 H444.3"/><path class="sv-head state" d="M442.3 228 l12 8 l-12 8z"/>
  </g><g class="sv-call c1">
    <rect class="sv-fill now" x="430.0" y="30" width="136.7" height="90"/>
    <path class="sv-line" d="M498.3 128 V200"/><path class="sv-head" d="M491.3 192 l7 10 l7 -10z"/>
    <rect class="sv-model" x="456.3" y="206" width="84" height="60" rx="8"/>
    <text x="498.3" y="244" class="sv-label strong">call 2</text>
  </g><g class="sv-state s1">
    <path class="sv-line state" d="M544.3 236 H581.0"/><path class="sv-head state" d="M579.0 228 l12 8 l-12 8z"/>
  </g><g class="sv-call c2">
    <rect class="sv-fill now" x="566.7" y="30" width="136.7" height="90"/>
    <path class="sv-line" d="M635.0 128 V200"/><path class="sv-head" d="M628.0 192 l7 10 l7 -10z"/>
    <rect class="sv-model" x="593.0" y="206" width="84" height="60" rx="8"/>
    <text x="635.0" y="244" class="sv-label strong">call 3</text>
  </g><g class="sv-state s2">
    <path class="sv-line state" d="M681.0 236 H717.7"/><path class="sv-head state" d="M715.7 228 l12 8 l-12 8z"/>
  </g><g class="sv-call c3">
    <rect class="sv-fill now" x="703.3" y="30" width="136.7" height="90"/>
    <path class="sv-line" d="M771.7 128 V200"/><path class="sv-head" d="M764.7 192 l7 10 l7 -10z"/>
    <rect class="sv-model" x="729.7" y="206" width="84" height="60" rx="8"/>
    <text x="771.7" y="244" class="sv-label strong">call 4</text>
  </g>
  <rect class="sv-block" x="20.0" y="30" width="136.7" height="90"/><rect class="sv-block" x="156.7" y="30" width="136.7" height="90"/><rect class="sv-block" x="293.3" y="30" width="136.7" height="90"/><rect class="sv-block" x="430.0" y="30" width="136.7" height="90"/><rect class="sv-block" x="566.7" y="30" width="136.7" height="90"/><rect class="sv-block" x="703.3" y="30" width="136.7" height="90"/>
  <path class="sv-wave" d="M20 75.0 L24 80.8 L28 86.4 L32 90.9 L36 93.5 L40 93.6 L44 91.0 L48 85.9 L52 78.9 L56 70.9 L60 63.0 L64 56.3 L68 51.9 L72 50.5 L76 52.5 L80 57.8 L84 65.6 L88 75.0 L92 84.7 L96 93.3 L100 99.5 L104 102.5 L108 101.7 L112 97.3 L116 89.7 L120 80.1 L124 69.9 L128 60.3 L132 52.8 L136 48.4 L140 47.7 L144 50.7 L148 56.9 L152 65.4 L156 75.0 L160 84.2 L164 91.9 L168 97.0 L172 98.9 L176 97.5 L180 93.2 L184 86.7 L188 78.9 L192 71.2 L196 64.5 L200 59.6 L204 57.2 L208 57.3 L212 59.8 L216 64.1 L220 69.4 L224 75.0 L228 80.0 L232 83.8 L236 86.0 L240 86.5 L244 85.4 L248 83.0 L252 79.9 L256 76.6 L260 73.5 L264 71.1 L268 69.6 L272 69.0 L276 69.3 L280 70.3 L284 71.8 L288 73.4 L292 75.0 L296 76.3 L300 77.3 L304 77.9 L308 78.0 L312 77.8 L316 77.3 L320 76.5 L324 75.5 L328 74.5 L332 73.4 L336 72.4 L340 71.7 L344 71.3 L348 71.4 L352 72.1 L356 73.3 L360 75.0 L364 77.1 L368 79.2 L372 81.1 L376 82.3 L380 82.7 L384 81.9 L388 79.9 L392 76.8 L396 73.0 L400 68.9 L404 65.2 L408 62.5 L412 61.2 L416 61.9 L420 64.6 L424 69.2 L428 75.0 L432 81.4 L436 87.5 L440 92.3 L444 95.0 L448 95.0 L452 92.1 L456 86.7 L460 79.2 L464 70.7 L468 62.4 L472 55.4 L476 50.9 L480 49.5 L484 51.7 L488 57.2 L492 65.3 L496 75.0 L500 84.9 L504 93.6 L508 99.9 L512 102.8 L516 101.9 L520 97.3 L524 89.7 L528 80.1 L532 69.9 L536 60.5 L540 53.1 L544 48.9 L548 48.3 L552 51.3 L556 57.5 L560 65.8 L564 75.0 L568 83.9 L572 91.2 L576 96.0 L580 97.7 L584 96.3 L588 92.1 L592 85.9 L596 78.7 L600 71.5 L604 65.2 L608 60.8 L612 58.7 L616 58.9 L620 61.2 L624 65.2 L628 70.0 L632 75.0 L636 79.4 L640 82.8 L644 84.6 L648 85.0 L652 84.0 L656 81.9 L660 79.2 L664 76.4 L668 73.8 L672 71.7 L676 70.5 L680 70.0 L684 70.3 L688 71.1 L692 72.3 L696 73.7 L700 75.0 L704 76.2 L708 77.0 L712 77.6 L716 77.8 L720 77.7 L724 77.3 L728 76.5 L732 75.6 L736 74.4 L740 73.2 L744 72.1 L748 71.2 L752 70.7 L756 70.7 L760 71.5 L764 72.9 L768 75.0 L772 77.4 L776 80.0 L780 82.2 L784 83.6 L788 84.0 L792 83.0 L796 80.7 L800 77.1 L804 72.7 L808 68.1 L812 64.0 L816 60.9 L820 59.7 L824 60.5 L828 63.6 L832 68.6 L836 75.0 L840 82.0"/>
  <path class="sv-line state" d="M200 325 H236"/><text x="246" y="331" class="sv-label left">state: the past, carried along</text>
</svg>
  </div>
</div>

Our model is **stateful**<br>
ONNX keeps that state in the open: `audio` + `state_in` → `audio_out` + `state_out`

Note:
- Some models read the past at every call, others remember
- Stateless: no memory, the past is handed in at every call
- Stateful: only the new block, carries the past along. 
- Our model is stateful, so we need to pass our memory
- Although ONNX works for a stateful model, it can't hide our state
- The export moves it out, so we need an explicit input and output
- Without passing the state, every block is the first block.

---

<div class="tag">Helpers: <code>onnx_engine.h</code></div>

## The engine<br><span class="then">The same call, with one more argument.</span>

```cpp
OnnxEngine engine(path);

engine.process(audio, num_samples);                     // without the state
engine.process(audio, num_samples, state, state_size);  // with the state of 63488 floats
```

Note:
- Let's have one more look at the engine, as I skipped a detail in step 1
- There is a second process function that takes 3 arguments. 
- Third argument: the state, after the audio, the same order as the model's inputs
- If no state is passed it defaults to zeros, that's why we got silence at the beginning of every block.
- The state is overwritten in place, in the same buffer, so the next call continues where this one ended

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="5"></div>

Fill in the TODOs in `steps/02_state/exercise/main.cpp`

```bash
cmake --build --preset release --target step02_exercise
./build/bin/step02_exercise
```

<div class="run" style="grid-template-columns: 1fr 1.5fr">
  <div class="run-step" data-seconds="2.2">
    <div class="run-title"><span>1</span>A vector for the state</div>
    <div class="run-visual run-size-buffers">
      <div class="mini-buf grow"></div>
      <div class="run-size">? floats</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3.4">
    <div class="run-title"><span>2</span>Pass it with the audio</div>
    <div class="run-visual run-wave">
      <div class="run-target">{{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=120|from=2|to=6}}</div>
      <div class="run-reveal">{{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=120|from=2|to=6}}</div>
      <div class="run-window"></div>
      <div class="run-ok">✓ every block matches</div>
    </div>
  </div>
</div>

Note:
- Two TODOs: above the loop, a vector for the state; in the loop, pass it with the audio
- We don't create the state ourselves: the model writes it on every call. We only give it the memory, zeros for the start
- 5 minutes, don't worry if you don't finish
- to build and run, use commands on the slide
- We solve it together, solutions are in the solution's main.cpp

---

<div class="tag">Solution</div>

## TODO 1: A vector for the state

```cpp
// given outside the function, in the global scope
constexpr struct {
    const char* m_path = WORKSHOP_ONNX_MODEL_PATH;
    size_t m_input_size = 2048;
    size_t m_state_size = 63488;
} k_model{};
```

```cpp
std::vector<float> state(k_model.m_state_size, 0.0f); // <- fill out
```

Note:
- correct size: given in the struct or 63488 floats, all zeros: a zero state is a new stream
- The vector lives outside the loop: made again per block, it would start at zero every time

---

<div class="tag">Solution</div>

## TODO 2: Pass it with the audio

```cpp
for (size_t i = 0; i < num_blocks; ++i) {
    engine.process(audio.data() + i * process_size, process_size,
                   state.data(), state.size());  // <- fill out
}
```

Note:
- Need to pass the third argument: the state
- It is overwritten on every call, so the next block continues where this one ended
- Every block now matches, to below 1e-6

---

<div class="tag">Done</div>

## Running every block and passing a state<br><span class="then">Now it sounds right.</span>

<div class="wave-player" data-src="assets/audio/demo_output_with_state.wav" data-blocks="1.48608@1.39556"></div>

<!-- <p class="wave-question fragment">Same model, same audio. Only the state is different.</p> -->



Note:
- If we listen to our example from before but now with the state passed on every block
- No more drop-outs: the sound runs through
- Same model, same audio, only the state is different
- Same output as Python, for every block
- However this is in the case we always get blocks of 2048 samples. But a host doesn't always hand us 2048 samples. Which brings us to step 3
