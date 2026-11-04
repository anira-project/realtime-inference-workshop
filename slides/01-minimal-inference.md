<h1>Step 1<br>Minimal C++ inference</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- First step: a minimal C++ program that runs the model
- Ignore real-time for now

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
- Goal: run the model in C++
- Load it with LibTorch
- Run a test signal through, block by block
- Compare with the Python output

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
- Model: TorchScript file, graph + weights
- Input signal + Python reference, compiled in as headers: no audio files, no Python
- Helper: the engine wrapper
- Only edit `main.cpp` in steps/01_minimal_inference

**Script:**

"To do this exercise we give you a few files you can work with. You'll get the model as a TorchScript file, which holds the graph and the weights. We will also give you an input signal and a reference output from Python, compiled in as headers. So we won't have any audio files and no Python to worry about. As a helper you get an engine wrapper. To do the exercise you only need to edit the `main.cpp` under steps/01_minimal_inference."

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
- First block almost silent: the model's own latency (step 6)

**Script:**

"First, let's have a look at the signals. The input is a 220 Hz sine. The reference is what the model made of it in Python. It's much quieter than the input and it isn't a sine anymore: this is what a model trained on a piano makes of a sine. The very first block, which we don't show here, is almost silent. That's the model's own latency. We will get into that in step 6."

---

<div class="tag">Helpers: <code>libtorch_engine.h</code></div>

## The engine<br><span class="then">The interface has three methods.</span>

```cpp
LibTorchEngine engine;
engine.load_model(path);              // Loads the model
engine.process(samples, num_samples); // One block, processed in place
engine.reset();                       // Clears the model's state
```

Note:
- Three methods: `load_model`, `process`, `reset`
- `reset` wipes the model's memory: its convolutions keep history from past blocks (more in step 7)
- Remember these three
- Later, what changes: who calls `process`, and from which thread

**Script:**

"Let's take a deeper look into our helper file. The engine has three methods: `load_model`, `process`, `reset`. `load_model` loads the model. `process` processes the audio one block at a time. `reset` clears the model's state, meaning it wipes its memory. The model has convolutional layers that "remember" information from past blocks. Its convolutions look back into the past, so internally it keeps the end of the previous block(s): cached history buffers inside the network. We will get deeper into stateful and stateless models in step 7. Remember those
three methods, since we will need them throughout the workshop. What changes later is who calls `process`, and from which thread."

---

<div class="tag">Helpers: <code>libtorch_engine.h</code> — inside</div>

## The engine<br><span class="then">Load once, then one block at a time.</span>

```cpp
// load_model()
m_model = torch::jit::load(model_path);
m_model.eval();

// process()
const torch::NoGradGuard no_grad;
const auto input  = torch::from_blob(samples, {1, 1, length}, torch::kFloat32);
const auto output = m_model.forward({input}).toTensor().contiguous();
std::copy_n(output.data_ptr<float>(), num_samples, samples);
```

Note:
- `load_model`: `torch::jit::load`, structure + weights, no rebuilding in C++
- 127 MB, so load once and keep the engine
- `eval()`: not training anymore
- Every block: `NoGradGuard` skips the learning bookkeeping
- `from_blob`: wraps our buffer, no copy. Shape: 1 batch, 1 channel, N samples
- `forward`: runs the block, updates the memory
- Copy the result back: in place, like an audio callback

**Script:**

"Let's take a quick look inside the engine. In load_model we load the model with torch::jit::load. That file contains everything: the structure of the network and all the trained weights, so we don’t need to rebuild the model in C++. Loading it takes a moment, it’s 127 megabytes, so we do it once and keep the engine around. eval() tells the model: we’re not training anymore, we’re only using you.

Then for every block: the NoGradGuard also says we're not training, so LibTorch can skip all the bookkeeping it would need for learning.

from_blob takes our sample buffer and wraps a tensor around it, without copying anything. The shape is one batch, one channel, and our number of samples.

Then forward runs the model on that block. It also updates the model’s memory, so the next block picks up right where this one ended.

And finally we copy the result back into our buffer, so it’s processed in place, just like an audio callback."

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
- Three TODOs: create the engine, pick the block size, run each block
- Build commands are on the slide
- 10 minutes, don't worry if you don't finish
- We solve it together, solutions are in the solution's main.cpp
- Walk around: most problems are build problems, not code problems

**Script:**

"So now we get to our first task. There are three TODOs in the exercise file. First create the engine, then pick the block size, and finally run each block.
The build commands are on the slide. You get 10 minutes to try and solve it. Don't worry if you don't make it. We will go through the solutions together and you can find the solutions in the solutions main.cpp too."

*Timebox (10 min). Walk around. Most problems here are build problems,
not code problems.*

---

<div class="tag">Solution</div>

## TODO 1: Create the engine

```cpp
LibTorchEngine engine;
const std::string model_path = k_model.m_path; // <- fill out

if (!std::filesystem::exists(model_path)) {
    std::fprintf(stderr, "TODO 1: load the model first, with the path to it\n");
    return 2;
}
engine.load_model(model_path);
```

Note:
- Only the path to fill in, but look where the engine sits: outside the loop
- Inside the loop: slow, loads 127 MB every block
- Inside the loop: wrong, throws away the state every block
- The engine lives as long as the stream

**Script:**

"Okay, let's look at the solutions together. All you had to fill in is the path, but notice where the engine sits: outside the loop. Inside the loop it would be slow, because you'd load 127 MB every block, and wrong, because you'd throw away the model's state every block. The engine lives as long as the stream."

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
- Try 512: the engine says the model returned 2048 samples for 512 in

**Script:**

"The correct blocksize is 2048, which is set by the model at export time. You can try to give the model 512, or 4096, and the model
itself will tell you that it takes exactly 2048 samples per call."

---

<div class="tag">Solution</div>

## TODO 3: Run each block

```cpp
for (size_t i = 0; i < num_blocks; ++i) {
    const size_t start = i * process_size;

    engine.process(output.data() + start, process_size); // <- fill out
}
```

Note:
- A plain loop, runs the model in place
- `output` starts as a copy of the input
- `process()` reads the samples and overwrites them with the result

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
- Same output as Python: it works
- But a host doesn't hand us 2048 samples, ours hands us 512: step 2

**Script:**

"The results show us: we get the same output as Python. It works. But a host doesn't hand us 2048 samples; ours hands us 512 at a time. Which brings us to step 2."
