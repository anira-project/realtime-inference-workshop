<h1>Step 1<br>Minimal C++ inference</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- First step: a minimal C++ program that runs the model
- One block, and ignore real-time for now

---

<div class="tag">Goal</div>

## The goal<br><span class="then">Run the model in C++.</span>

<div class="statement-points">
  <div><small>1</small>Load the model with ONNX Runtime</div>
  <div><small>2</small>Run one block of a test signal through the model</div>
  <div><small>3</small>Compare the result with the model in Python</div>
</div>

Note:
- Goal: run the model in C++
- Load it with ONNX Runtime
- Run one block of a test signal through it
- Compare with the Python output

---

<div class="tag">Given</div>

## What we get<br><span class="then">3 assets, 1 helper, 1 file to edit.</span>

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">common/assets/</div>
    {{FILE:model_forward.onnx}}
    {{FILE:test_signal.h}}
    {{FILE:target_signal.h}}
  </div>
  <div class="given-column">
    <div class="given-label">Helpers</div>
    <div class="given-folder">common/helpers/</div>
    {{FILE:onnx_engine.h}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">01_minimal_inference/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
- Model: ONNX file, graph + weights
- Input signal + Python reference, compiled in as headers: no audio files, no Python
- Helper: the engine wrapper
- Only edit `main.cpp` in steps/01_minimal_inference

---

<div class="tag">Assets: <code>test_signal.h</code> & <code>target_signal.h</code></div>

## Input: a sine<br><span class="then">Reference: the model's output in Python.</span>

<div class="signal-pair">
  <div class="signal-label"><code>k_input_signal</code></div>
  {{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=230|from=5|to=7}}
  <div class="signal-label"><code>k_target_output_signal</code></div>
  {{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=230|from=5|to=7}}
</div>

Note:
- Input: 220 Hz sine
- Reference: what the model makes of it in Python

---

<div class="tag">Helpers: <code>onnx_engine.h</code></div>

## The engine<br><span class="then">Load the model, then process the data.</span>

```cpp
OnnxEngine engine(path);                  // Loads the model

engine.process(samples, num_samples);     // Overwrites the block in place
```

Note:
- The onnx engine wrapper has a constructor and one method, `process`
- Constructor loads the model using the model_path
- `process` takes where the block starts and how many samples it has
- The audio is processed in place

---

<div class="tag">Helpers: <code>onnx_engine.h</code> — inside</div>

## The engine<br><span class="then">Load once, then one block at a time.</span>

```cpp
// constructor
m_session = Ort::Session(m_env, model_path.c_str(), options);
m_inputs.push_back(create_tensor(m_audio_in, m_input_shapes[0]));
m_outputs.push_back(create_tensor(m_audio_out, m_input_shapes[0]));

// process()
std::copy_n(samples, num_samples, m_audio_in.begin());

m_session.Run(Ort::RunOptions{nullptr}, ..., m_inputs.data(), ..., m_outputs.data(), ...);

std::copy_n(m_audio_out.begin(), num_samples, samples);
```

Note:
- The constructor creates an ONNX Runtime session `Ort::Session`. 
- The `.onnx` file contains everything: the structure of the network and all the trained weights, so we don't need to rebuild the model in C++.
- Model is 126 MB, so load once and keep the engine
- Tensors created once, each wrapping a buffer of the engine. Shape: 1 batch, 1 channel, N samples
- Every block: copy in, `Run`, copy out
- `Run` writes into the output tensors we already have
- From the outside still in place, like an audio callback

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="8"></div>

Fill in the TODOs in `steps/01_minimal_inference/exercise/main.cpp`

```bash
cmake --preset release      # once
cmake --build --preset release --target step01_exercise
./build/bin/step01_exercise
```

<div class="run" style="grid-template-columns: 1fr 1.5fr 1.5fr">
  <div class="run-step" data-seconds="2">
    <div class="run-title"><span>1</span>Set the model path</div>
    <div class="run-visual">
      <div class="run-file">{{FILE:model_forward.onnx}}</div>
      <div class="run-engine">{{ICON:cpp}}<small>ONNX Runtime</small></div>
    </div>
  </div>
  <div class="run-step" data-seconds="2.2">
    <div class="run-title"><span>2</span>Pick the block size</div>
    <div class="run-visual run-wave">
      {{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=120|from=2|to=6}}
      <div class="run-cuts"><i></i><i></i><i></i></div>
      <div class="run-size">? samples</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3.4">
    <div class="run-title"><span>3</span>Run the first block</div>
    <div class="run-visual">
      <div class="run-engine">{{ICON:cpp}}<small>process()</small></div>
      <div class="run-ok">✓ matches the reference</div>
    </div>
  </div>
</div>

Note:
- Three TODOs: set the model path, pick the block size, run the first block
- Build commands are on the slide
- 8 minutes, don't worry if you don't finish
- to build and run, use commands on the slide
- We solve it together, solutions are in the solution's main.cpp
- Walk around: most problems are build problems, not code problems

---

<div class="tag">Solution</div>

## TODO 1: Set the model path

```cpp
// given outside the function, in the global scope
constexpr struct {
    const char* m_path = WORKSHOP_ONNX_MODEL_PATH;
    size_t m_input_size = 2048;
} k_model{};
```

```cpp
const std::string model_path = k_model.m_path; // <- fill out
```

Note:
- Only the path to fill in
- The engine loads it once: 126 MB, too slow to do per block

---

<div class="tag">Solution</div>

## TODO 2: Pick the block size

<div class="run-visual run-wave run-solved" style="margin-bottom: 5rem">
  {{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=120|from=2|to=6}}
  <div class="run-cuts"><i></i><i></i><i></i></div>
  <div class="run-size"><span class="was">? samples</span><span class="is">2048 samples</span></div>
</div>

```cpp
const size_t process_size = k_model.m_input_size; // <- fill out
```

Note:
- 2048, set by the model at export
- Try 512: the engine says the model takes exactly 2048 samples per call

---

<div class="tag">Solution</div>

## TODO 3: Run the first block

```cpp
std::vector<float> audio(k_input_signal.begin(), k_input_signal.end());

engine.process(audio.data(), process_size); // <- fill out

return report_first_block(audio, k_input_signal, k_target_output_signal, process_size);
```

Note:
- `audio` starts as a copy of the input
- `process()` reads the block and overwrites it with the result
- Only the first `process_size` samples are compared

---

<div class="tag">Done</div>

## AI model integrated in C++

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
- Same output as Python within 1e-06 difference to the reference: it works, for one block
- Next: let's see what happens if we run the whole signal

---

<div class="tag">Listen</div>

## Running every block<br><span class="then">Something sounds wrong.</span>

<div class="wave-player" data-src="assets/audio/demo_output_state_cleared.wav" data-blocks="1.48608@1.39556"></div>

<p class="wave-question fragment">Why does this happen?</p>

Note:
- One block works. Now we run the whole signal, block after block: listen
- the sound keeps cutting out. Something is wrong
- Click: why? That's step 2

---

<div class="tag">Problem</div>

## Running every block<br><span class="then">Only the first one matches.</span>

```cpp
for (size_t i = 0; i < num_blocks; ++i) {
    engine.process(audio.data() + i * process_size, process_size);
}
```

```
block 0    max abs diff 2.33e-10   OK
block 1    max abs diff 0.0802     <-- FAILED
block 2    max abs diff 0.0791     <-- FAILED
block 3    max abs diff 0.053      <-- FAILED
block 4    max abs diff 0.113      <-- FAILED
...
```

Note:
- What the exercise prints for the whole signal: block 0 matches, every block after it fails
- Don't explain yet: that's step 2
