<h1>Step 1b<br>Stateless and stateful</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- Step 1a ran one block
- Now the whole signal, and what the model needs for that

**Script:**

"In step 1a we ran one block, and it matched. Now we run the whole signal, block by block."

---

<div class="tag">Listen</div>

## Passing only the audio<br><span class="then">Every block, with nothing else.</span>

<div class="spectro-player" data-a="assets/audio/demo_input.wav" data-b="assets/audio/demo_output_no_state.wav"
     data-label-a="In" data-label-b="Out — only the audio passed"></div>

Note:
- The same call as in step 1a, once per block, on the demo chords
- Fade across: the output is practically silent, peak around -75 dB
- Not clicks or dropouts: nothing comes out at all
- Why? Next slide

**Script:**

"Let's do exactly what we did in step 1a, once per block, on the chords you heard earlier. Here's what goes in. And when I fade across to what comes out... nothing. It's practically silent, around minus 75 dB. You might expect clicks or dropouts, but there is simply nothing there. So why does the model stop working?"

---

<div class="tag">Concept</div>

## Stateless vs. stateful<br><span class="then">Some models remember, others read the past at every call.</span>

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

Our model is stateful, and ONNX keeps that state in the open:<br>
`audio` + `state_in` → `audio_out` + `state_out`

Note:
- Stateless: no memory, the past is handed in at every call
- Stateful: only the new block, carries the past along (the dashed line). Our model
- Consequences: calls in order, one state per stream, zeros for a new stream
- ONNX can't hide state: the export moves it out, so it's an explicit input and output
- Without it, every block is the first block. Latency is 2224 samples and a block is 2048, so a first block is nearly silent

**Script:**

"A stateless model has no memory, so the past has to be handed in again at every call. A stateful model only gets the new block, and it carries the past along from one call to the next. That's the dashed line, and it's what our model does.

That memory has consequences: the calls have to be in order, you need one state per stream, and a new stream starts with a zero state.

ONNX can't hide the state inside the model, so the export moves it out. It's an explicit input and output: we hand it in with every block and get the updated state back. If we don't, every block is the first block of a new stream. And our model takes longer than one block to answer, so a first block is nearly silent. That is what you just heard."

---

<div class="tag">Helpers: <code>onnx_engine.h</code></div>

## The engine<br><span class="then">The same call, with one more buffer.</span>

```cpp
OnnxEngine engine(path);

// {audio, state}:          the two buffers
// {samples, state size}:  their sizes
engine.process({audio, state}, {num_samples, state_size}, start);
```

- The block at `start` in `audio` is overwritten with the model's audio
- `state` is overwritten with the new state
- The state is 63488 floats, starting at zero

Note:
- This is why `process` takes lists
- Same order as the model's inputs, and as in step 9
- The state comes back in the same buffer, so the next call continues
- 63488 floats: every convolution cache in the model

**Script:**

"This is why `process` takes lists. The first buffer is the audio, and the second one can be the state. The state is overwritten on every call, so the next call continues where this one ended. The second list has one size per buffer: the block size and the size of the state. The state is 63488 floats, every convolution cache in the model, and it starts at zero."

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="12"></div>

Create the state, then pass the audio, the state and their sizes, in `steps/01b_state/exercise/main.cpp`

```bash
cmake --build --preset release --target step01b_exercise
./build/bin/step01b_exercise
```

The state is **63488** floats, starting at zero.

```
block 0     max abs diff 2.33e-10   OK
block 1     max abs diff 0.0802     <-- FAILED
```

Note:
- Two TODOs: create the state above the loop, pass it in the loop
- It runs as it is, and fails from block 1 on
- Block 0 passes: a zero state is correct for the first block
- 12 minutes

**Script:**

"There are two TODOs. Above the loop, create the state. In the loop, pass the audio and the state, and their sizes. The state size is on the slide. It runs as it is, and you'll see block 0 pass, because a zero state is correct for the first block, and everything after it fail. You get 12 minutes."

*Timebox (12 min). Walk around.*

---

<div class="tag">Solution</div>

## The audio and the state

```cpp
std::vector<float> state(k_model.m_state_size, 0.0f);  // <- fill out

for (size_t i = 0; i < num_blocks; ++i) {
    engine.process({audio, state},  // <- fill out
                   {process_size, k_model.m_state_size},
                   i * process_size);
}
```

Note:
- One more buffer, one more size
- The state is created above the loop: created per block, it would start at zero every time
- Every block now matches, to below 1e-6

**Script:**

"All it takes is one more buffer and one more size. Note that the state is created above the loop: if you created it again for every block, it would start at zero every time, and you'd be back to where we started. Now every block matches the reference."

---

<div class="tag">Listen</div>

## The audio and the state<br><span class="then">Now the model answers.</span>

<div class="spectro-player" data-a="assets/audio/demo_output_no_state.wav" data-b="assets/audio/demo_output.wav"
     data-label-a="Without state" data-label-b="With state"></div>

Note:
- Fade across: from silence to the model's output
- Same model, same audio, only the state is different

**Script:**

"Here's the same model with the same input. On one side only the audio is passed, and on the other side the audio and the state. Same model, and it's only the state that makes the difference."

---

<div class="tag">Done</div>

## The model runs the whole signal

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
- Same output as Python, for every block
- But a host doesn't always hand us 2048 samples: step 2

**Script:**

"Same output as Python, for every block. But a host doesn't always hand us 2048 samples. Which brings us to step 2."
