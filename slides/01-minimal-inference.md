<h1>Step 1<br>Minimal C++ inference</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
    - First hands-on block. Everyone builds and runs before we talk about speed.
    - The first step will be to have a minimal C++ script that implements the model and runs it. 
    -We will ignore real-time concerns for now

**Script:**

"The first step will be to have a minimal C++ script that implements the model and runs it. We will ignore real-time concerns for now."

---

<div class="tag">Goal</div>

## The goal<br><span class="then">Run the model in C++.</span>

<div class="statement-points">
  <div><small>1</small>Load the model with LibTorch</div>
  <div><small>2</small>Run a test signal through the model</div>
  <div><small>3</small>Compare results with the model in Python</div>
</div>

Note:

**Script:**

"So the goal is to run the model in C++. First we have to load the model with LibTorch. Then we run a test signal through it block by block. To make sure it worked we compare the results with the output that we get in Python."


---

<div class="tag">Given</div>

## What we get<br><span class="then">3 assets, 1 helper, 1 file to edit.</span>

<div class="given">
  <div class="given-column">
    <div class="given-label">Assets</div>
    <div class="given-folder">models/ & common/assets/</div>
    {{FILE:model.pt}}
    {{FILE:test_signal.h}}
    {{FILE:target_signal.h}}
  </div>
  <div class="given-column">
    <div class="given-label">Helpers</div>
    <div class="given-folder">common/helpers/</div>
    {{FILE:libtorch_engine.h}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">01_minimal_inference/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
    - model.pt: the model, TorchScript, graph + weights, 127 MB; stateful, the state lives inside it.
    - WORKSHOP_MODEL_PATH: set by CMake (-DWORKSHOP_MODEL=...), read in main.cpp as k_model.m_path.
    - support.h (report_line, k_tolerance) is plumbing for the checks and stays off the slides.
    - test_signal.h / target_signal.h: compiled in, no audio files, no Python.

**Script:**

"To do this excercise we give you a files you can work with. You'll get the model as a TorchScript file, which holds the graph and the weights. We will aslo give an input signal and a reference output compiled in Python as headers. So we won't have any audio files and no Python to worry about. As a helper you get an engine wrapper. To do the exercise you only need to edit the `main.cpp` under steps/01_minimal_inference."

---

<div class="tag">Assets: <code>test_signal.h</code> & <code>target_signal.h</code></div>

## Input: a sine<br><span class="then">Reference: the model's output in Python.</span>

<div class="signal-pair">
  <div class="signal-label"><code>k_input_signal</code></div>
  {{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=230|from=2|to=4}}
  <div class="signal-label"><code>k_target_output_signal</code></div>
  {{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=230|from=2|to=4}}
</div>

Note:
    - Input: 220 Hz sine at half scale, 8 blocks of 2048 samples at 44.1 kHz; shown here: blocks 3 and 4, the dashed line between them is a block boundary.
    - Target: what the model made of it, in Python. The first block is nearly silent — the model's latency has not filled yet.

**Script:**

"First, let's have a look at the signals. The input is a 220 Hz sine. The reference is what the model made of it in Python. Notice that the first block is almost silent. That's the model's own latency. We will get into that in step 7."

---

<div class="tag">Helpers: <code>libtorch_engine.h</code></div>

## The engine<br><span class="then">The interface has three methods.</span>

```cpp
LibTorchEngine engine(path);          // Loads the model
engine.process(samples, num_samples); // One block, processed in place
engine.reset();                       // Clears the model's state
```

Note:
    - What changes later is who calls process(), and from which thread.

**Script:**

"Let's take a deeper look into ur helper file. The engine has three methods: construct, `process`, `reset`. the constructor loads the model. `process` processes the audio one block at the time. `reset` clears the models state, meaning it wipes it's memory. The model has convolutional layers that "remeber" information from past blocks. Its convolutions look back into the past, so internally it keeps the end of the previous block(s): cached history buffers inside the network. We will get deeper into stateful and stateless models in step 4. Remember those
three methods, since we will need them throughout the workshop."

---

<div class="tag">Helpers: <code>libtorch_engine.h</code> — inside</div>

## The engine<br><span class="then">Load once, then one block at a time.</span>

```cpp
// Constructor
m_model = torch::jit::load(model_path);
m_model.eval();

// process()
const torch::NoGradGuard no_grad;
const auto input  = torch::from_blob(samples, {1, 1, length}, torch::kFloat32);
const auto output = m_model.forward({input}).toTensor().contiguous();
std::copy_n(output.data_ptr<float>(), num_samples, samples);
```

Note:
    - load: a TorchScript file carries graph and weights; no model class in C++. eval(): inference mode.
    - NoGradGuard: no autograd graph, no allocations for it.
    - from_blob wraps your buffer, no copy — the buffer must outlive the tensor. Shape {batch, channels, samples}.
    - forward also advances the model's state; contiguous() guarantees a flat layout to read from.
    - copy_n writes back in place.

**Script:**

"Let's take a quick look inside the engine. In the constructor we load the model with torch::jit::load. That file contains everything: the structure of the network and all the trained weights, so we don’t need to rebuild the model in C++. Loading it takes a moment, it’s 127 megabytes, so we do it once and keep the engine around. eval() tells the model: we’re not training anymore, we’re only using you.

Then for every block: the NoGradGuard also says we're not training, so LibTorch can skip all the bookkeeping it would need for learning.

from_blob takes our sample buffer and wraps a tensor around it, without copying anything. The shape is one batch, one channel, and our number of samples.

Then forward runs the model on that block. It also updates the model’s memory, so the next block picks up right where this one ended.

And finally we copy the result back into our buffer, so it’s processed in place, just like an audio callback.

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="5"></div>

Fill in the TODOs in `steps/01_minimal_inference/exercise/main.cpp`

```bash
cmake --preset release      # once
cmake --build --preset release --target step01_exercise
./build/bin/step01_exercise
```

<div class="run" style="grid-template-columns: 1fr 1.5fr 1.5fr">
  <div class="run-step" data-seconds="2">
    <div class="run-title"><span>1</span>Create the engine</div>
    <div class="run-visual">
      <div class="run-file">{{FILE:model.pt}}</div>
      <div class="run-engine">{{ICON:cpp}}<small>LibTorch</small></div>
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
    <div class="run-title"><span>3</span>Run each block</div>
    <div class="run-visual run-wave">
      <div class="run-target">{{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=120|from=2|to=6}}</div>
      <div class="run-reveal">{{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=120|from=2|to=6}}</div>
      <div class="run-window"></div>
      <div class="run-ok">✓ matches the reference</div>
    </div>
  </div>
</div>

Note:
**Script:**

"So now we get to our first task. There are three TODOs in the exercise file. First create the engine., then pick the block size, and finally run each block.
The build commands are on the slide. You get 5 minutes to try and solve it. Don't worry if you don't make it. We will go through the solutions together and you can find the solutions in the solutions main.cpp too."

*Timebox (e.g. 5 min). Walk around. Most problems here are build problems,
not code problems.*

---

<div class="tag">Solution</div>

## TODO 1: Create the engine</span>

```cpp
std::unique_ptr<LibTorchEngine> engine;
try {
    engine = std::make_unique<LibTorchEngine>(k_model.m_path);
} catch (const std::runtime_error& error) {
    std::fprintf(stderr, "%s\n", error.what());
    return 2;
}
```

Note:
    - Outside the loop. Created inside, block 1 is already wrong: the state was thrown away.
    - Slow, too: 127 MB read per block. The engine lives as long as the stream.

**Script:**

" Okay, let's look at teh solutions together. The common mistake is creating the engine inside the loop. That's slow because
you load 127 MB every block, and it's also wrong because you throw away the
model's state every block. The engine lives as long as the stream. So make sure to create the enging outside the loop."

---

<div class="tag">Solution</div>

## TODO 2: Pick the block size

<div class="run-visual run-wave run-solved" style="margin-bottom: 5rem">
  {{WAVEFORM:steps/common/assets/test_signal.h|blocks=2048|height=120|from=2|to=6}}
  <div class="run-cuts"><i></i><i></i><i></i></div>
  <div class="run-size"><span class="was">? samples</span><span class="is">2048 samples</span></div>
</div>

```cpp
const size_t process_size = static_cast<size_t>(k_model.m_input_size);  // 2048
```

Note:
    - The model sets it, fixed at export.
    - Smaller sizes: the traced model still hands back 2048 samples, and the engine stops with "the model returned 2048 samples for 512 in".
    - Multiples of 2048 happen to go through and match here; nothing in the export promises that.

**Script:**

"The correct blocksize is 2048, which is set by the model at export time. You can try to give the model 512 and the engine will tell you the
model returned 2048 samples for 512 in."

---

<div class="tag">Solution</div>

## TODO 3: Run each block

```cpp
for (size_t i = 0; i < num_blocks; ++i) {
    engine->process(output.data() + i * process_size, process_size);
}
```

Note:
    - In place: afterwards output holds what the model made of the signal.

**Script:**

"Well this was quite straight forward. We need a plain loop, that runs the model in place. `output` starts out as a copy of the input, and process() works in place: it reads the samples from the buffer and overwrites them with the model’s result."

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
    - Same output as in Python. Whether it is fast enough is still open.
    - Straight into step 2: the host's block size.

**Script:**

"The result show us: we get the same output as Python. It works. But a host doesn't always hand us 2048 samples. Which brings us to step 2."
