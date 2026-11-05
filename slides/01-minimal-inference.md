<h1>Step 1<br>Minimal C++ inference</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- First step: a minimal C++ program that runs the model
- One block, and ignore real-time for now

**Script:**

"The first step will be a minimal C++ program that loads the model and runs one block through it. We will ignore real-time concerns for now."

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

**Script:**

"So the goal is to run the model in C++. First we load the model with ONNX Runtime. Then we run one block of a test signal through it. To make sure it worked, we compare the result with the output that we get in Python."

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

**Script:**

"To do this exercise we give you a few files you can work with. You'll get the model as an ONNX file, which holds the graph and the weights. We will also give you an input signal and a reference output from Python, compiled in as headers. So we won't have any audio files and no Python to worry about. As a helper you get an engine wrapper. To do the exercise you only need to edit the `main.cpp` under steps/01_minimal_inference."

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
- Much quieter, not a sine anymore: a piano model's take on a sine

**Script:**

"First, let's have a look at the signals. The input is a 220 Hz sine. The reference is what the model made of it in Python. It's much quieter than the input and it isn't a sine anymore: this is what a model trained on a piano makes of a sine."

---

<div class="tag">Helpers: <code>onnx_engine.h</code></div>

## The engine<br><span class="then">Load the model, then process the data.</span>

```cpp
OnnxEngine engine(path);                  // Loads the model

engine.process(samples, num_samples);     // Overwrites the block in place
```

Note:
- Constructor loads the model
- `process` takes where the block starts and how many samples it has
- The audio is processed in place
- The model takes more than audio: step 2

**Script:**

"Let's take a look at our helper file. The constructor loads the model. `process` takes where the block starts and how many samples it has, just like an audio callback. It works in place, so the model's output overwrites the block. The model actually takes more than audio, and we will come back to that in step 2."

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
- `Ort::Session`: structure + weights, no rebuilding in C++
- 126 MB, so load once and keep the engine
- Tensors created once, each wrapping a buffer of the engine. Shape: 1 batch, 1 channel, N samples
- Every block: copy in, `Run`, copy out
- `Run` writes into the output tensors we already have
- From the outside still in place, like an audio callback

**Script:**

"Let's take a quick look inside the engine. The constructor creates an ONNX Runtime session. That file contains everything: the structure of the network and all the trained weights, so we don't need to rebuild the model in C++. Loading it takes a moment, it's 126 megabytes, so we do it once and keep the engine around.

The constructor also creates the tensors, once. Each one wraps a buffer owned by the engine. The audio shape is one batch, one channel, and our number of samples. There is a second pair of tensors too, not shown here, and we will get to that in step 2.

Then for every block we copy the samples into the input buffer and call Run, which writes into the output tensors we already have. And finally we copy the result back into our buffer, so from the outside it's processed in place, just like an audio callback."

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
- We solve it together, solutions are in the solution's main.cpp
- Walk around: most problems are build problems, not code problems

**Script:**

"So now we get to our first task. There are three TODOs in the exercise file. First set the model path, then pick the block size, and finally run the first block.
The build commands are on the slide. You get 8 minutes to try and solve it. Don't worry if you don't make it. We will go through the solutions together and you can find the solutions in the solution's main.cpp too."

*Timebox (8 min). Walk around. Most problems here are build problems,
not code problems.*

---

<div class="tag">Solution</div>

## TODO 1: Set the model path

```cpp
const std::string model_path = k_model.m_path; // <- fill out
```

Note:
- Only the path to fill in
- The engine loads it once: 126 MB, too slow to do per block

**Script:**

"Okay, let's look at the solutions together. All you had to fill in is the path. The engine loads this file once, because at 126 MB it's far too slow to do it per block."

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

**Script:**

"The correct block size is 2048, which is set by the model at export time. You can try to give the model 512, or 4096, and the engine will tell you that the model takes exactly 2048 samples per call."

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

**Script:**

"Well this was quite straightforward. `audio` starts out as a copy of the input, and process() works in place: it reads the samples from the buffer and overwrites them with the model's result. Then we compare that one block with the reference."

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
- Same output as Python: it works, for one block
- Next: the whole signal, step 2

**Script:**

"The result shows us: we get the same output as Python. It works, for one block. But audio doesn't stop after one block. So let's run the whole signal, which brings us to step 2."
