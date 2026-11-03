<h1>Step 2<br>Model input size<br>vs. host buffer size</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- Ask: what buffer sizes do you ship with? Collect a few
- Wide range of host block sizes, the model always takes 2048
- Goal: make it work with any host block size

**Script:**

*Ask the room: "What buffer sizes do you ship with?" Collect a few
numbers: 64, 128, 256, 441...*
"As we can hear we have a wide range of host block sizes. The model takes 2048 samples, but the host can give us anything. So we need to make the model work with any host block size."

---

<div class="tag">Motivation</div>

## What is the problem? <span class="then">Two owners, two block sizes.</span>

<div class="streams">
  {{STREAM:host=64|model=2048|length=4096|seconds=7}}
  {{STREAM:host=441|model=2048|length=4096|seconds=7}}
</div>

Note:
- Two owners, two block sizes
- Model: fixed at export
- Host: device and user, can change while running
- 64: 31 callbacks nothing, the 32nd runs the whole model
- 441: doesn't divide 2048, the model runs at a different place each time
- They can match by coincidence, we can't count on it

**Script:**

"So our problem is, we have two owners, with two different block sizes. The model's block is fixed at export. The host's block comes from the device and the user, and it can even change while running. Let's look at an example with our model. At 64 samples e.g., nothing happens in the first 31 callbacks, because we haven't reached the models block size yet. The 32nd callback runs the whole model. For a host blocksize of 441 it doesn't even divide, 2048 is not a multiple of 441: the model will always run at a different place in the relevant callback.
Sometimes the block sizes will match by coincidence, but we cannot count on that happening."

---

<div class="tag">Approach</div>

## The approach<br><span class="then">A ring buffer on each side.</span>

<span> The host and the model each work in their own block size.</span>

<div class="pump" style="margin-bottom: 250px;">
  <div class="pump-node">host in<small>512 per call</small></div>
  <div class="pump-arrow"></div>
  <div class="pump-buffer in"><div class="pump-fill"></div><div class="pump-mark"></div></div>
  <div class="pump-arrow"></div>
  <div class="pump-node model">model<small>2048 at once</small></div>
  <div class="pump-arrow"></div>
  <div class="pump-buffer out"><div class="pump-fill"></div><div class="pump-mark"></div></div>
  <div class="pump-arrow"></div>
  <div class="pump-node">host out<small>512 per call</small></div>
</div>

Note:
- The model always gets 2048, the host always gets what it needs back in time
- A ring buffer on each side of the model
- Host pushes whatever it has into the first
- Full block in: the model runs, writes 2048 to the output ring buffer
- Host takes its block size each call, 512 in this picture

**Script:**

"So we need a reliable way of passing samples from the host to the model that makes sure the model will always get a block of 2048 and the host will always get the amount of samples it needs back in time. The approach is to use ring buffers on each side of the model. The host pushes whatever it has into the first ring buffer. Once a
full model block is in, the model runs and writes 2048 samples to the output
ring buffer. From there the host takes as many samples as its block size asks for at each call, 512 in this picture."

---

<div class="tag">Goal</div>

## The goal<br><span class="then">The model should work with any host block size.</span>

<div class="statement-points">
  <div><small>1</small>Create the ring buffers</div>
  <div><small>2</small>Pump whole blocks through the model</div>
  <div><small>3</small>Hand the host its samples back</div>
</div>

Note:
- Goal: works with any host block size
- Size the ring buffers
- Pump whole blocks through the model
- Hand the samples back to the host

**Script:**

"So, the goal is to make sure the system works with any host block size. For that we need to give the ring buffers a size, pump whole blocks through the model and then hand the samples back to the host."

---

<div class="tag">Given</div>

## What we get<br><span class="then">2 new helpers, 1 file to edit.</span>

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
    {{FILE:ring_buffer.h|new}}
    {{FILE:host.h|new}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">02_host_buffer_size/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
- Same model, signals and engine helper
- New: a ring buffer and a fake host
- The exercise isn't writing a ring buffer, it's sizing and using it

**Script:**

"We still have the same model and signal files, we also still have the engine helper file. But now we get 2 new helper files. One is a ring buffer and the other a fake host that we will use to solve the exercise. The exercise won't be to create a ring buffer yourself, but rather to handle it, so to give it the right size and use it."

---

<div class="tag">Helpers: <code>ring_buffer.h</code></div>

## The ringbuffer <span class="then">A fixed-size queue for samples.</span>

```cpp
RingBuffer buffer(capacity);           // Allocates once, never again
buffer.available();                    // Samples waiting to be read
buffer.space();                        // Room left for writing
buffer.push(samples, num_samples);     // Append at the back — throws if it does not fit
buffer.pop(samples, num_samples);      // Take from the front — throws if too few
```

<div class="ring">
  <div class="ring-slots"></div>
  <div class="ring-fill"></div>
  <div class="ring-head pop">pop</div>
  <div class="ring-head push">push</div>
</div>

Note:
- A queue for samples with a fixed size
- Allocates once, when created, never again
- `push` at the back, `pop` from the front: first in, first out
- Wrong size: it throws, so we notice right away

**Script:**

"Let's look at the ring buffer: a queue for samples with a fixed size. It allocates once, when we create it, and never again. We add samples at the back with push and take them from the front with pop, first in, first out. And if we size it wrong, it throws, so we notice right away instead of getting strange audio."

---

<div class="tag">Helpers: <code>host.h</code></div>

## The host <span class="then">A DAW, minus the DAW.</span>

```cpp
std::vector<float> out = run_host(signal, num_samples, block_size,
                                  [&](float* samples, size_t n) { /* your callback */ });
```

<div class="streams">
  {{STREAM:host=64|model=0|length=4096|seconds=5}}
  {{STREAM:host=512|model=0|length=4096|seconds=5}}
  {{STREAM:host=2048|model=0|length=4096|seconds=5}}
</div>

Note:
- A DAW without the DAW: signal + block size in, calls your callback block by block
- Overwrite the samples in place, like an audio callback
- Block size is just a parameter: many sizes in one run (64, 512, 2048 here)
- Goal: works at all block sizes, output matches the reference

**Script:**

"And this is our host. It’s a DAW without the DAW: you give it a signal and a block size, and it calls your callback block by block, just like an audio callback, so you overwrite the samples in place.

Because the block size is just a parameter, we can test lots of sizes in one run. Here you see 64, 512 and 2048. For this exercise we our goal is that your code works for all of the blocks sizes, and that the output is the same as the reference."

---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="10"></div>

Fill in the TODOs in `steps/02_host_buffer_size/exercise/main.cpp`

```bash
cmake --build --preset release --target step02_exercise
./build/bin/step02_exercise
```

<div class="run">
  <div class="run-step" data-seconds="2.2">
    <div class="run-title"><span>1</span>Give the ring buffers a size</div>
    <div class="run-visual run-size-buffers">
      <div class="mini-buf grow"></div>
      <div class="mini-buf grow"></div>
      <div class="run-size">? samples</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3.4">
    <div class="run-title"><span>2</span>Pump the whole blocks</div>
    <div class="run-visual">
      <div class="mini-buf pumping"><div class="mini-fill"></div><div class="mini-mark"></div></div>
      <div class="mini-model">model</div>
    </div>
  </div>
  <div class="run-step" data-seconds="3">
    <div class="run-title"><span>3</span>Hand back the samples </div>
    <div class="run-visual run-wave run-out">
      <div class="run-reveal">
        <div class="run-silence"></div>
        {{WAVEFORM:steps/common/assets/target_signal.h|blocks=2048|height=120|from=2|to=6}}
      </div>
      <div class="run-ok">✓ the same stream at every block size</div>
    </div>
  </div>
</div>

Note:
- Three things: size the ring buffers, pump a whole block, hand the samples back
- `main.cpp` in steps/02_host_buffer_size/exercise, build commands on the slide
- 5 minutes, don't worry if you don't finish, we solve it together
- The interesting TODO is the sizing: let people argue about it

**Script:**

"Now it's your turn again. We will now give the ring buffers the proper size. Then we will pump a whole block though our model and finally hand the samples back to the host. You'll find the main.cpp in steps/02_host_buffer_size/exercise. The build commands are on the slide. You get 5 minutes to try and solve it. Again, don't worry if you don't make it. We will go through the solutions together and you can find the solutions in the solutions main.cpp too."
*Timebox. The interesting TODO is the sizing. Let people argue about it.*

---

<div class="tag">Solution</div>

## TODO 1: Size the ring buffers

```cpp
const size_t capacity = max_block_size + 2 * m_model_input_size; // <- fill out
```

Note:
- Big enough for the worst case
- Worst case: host just wrote a block, model just produced one, previous one not drained yet
- Happens whenever the sizes don't divide
- `max_block_size + 2 * model_size`

**Script:**

"We need to give the ring buffers a size that will be big enough for our worst case scenario. So the worst case would be: the host just wrote a block, the model just produced one, and the previous one isn't drained yet. That happens whenever the sizes don't divide. That will be `max_block_size + 2 * model_size`."

---

<div class="tag">Solution</div>

## TODO 2: Pump whole blocks

```cpp
m_input.push(samples, num_samples); // <- 1. fill out

const size_t num_model_blocks = m_input.available() / m_model_input_size; // <- 2. fill out

for (size_t b = 0; b < num_model_blocks; ++b) {
    m_input.pop(m_model_block.data(), m_model_input_size); // <- 3. fill out
    m_engine.process(m_model_block.data(), m_model_input_size);
    m_output.push(m_model_block.data(), m_model_input_size); // <- 3. fill out
    …
}
```

Note:
- Only pump with at least 2048 samples
- Push the host's samples into the input ring buffer
- Count whole model blocks waiting: available / 2048
- Can be more than one, if the host block is bigger than the model block
- For each: pop, process, push to the output ring buffer
- The rest waits for the next callback

**Script:**

"We should only pump the block when we have at least 2048 samples. So we need to check how many samples are in our input ring buffer. So first we push the host's samples into the input ring buffer, and then we count how many whole model blocks are waiting: available divided by 2048. That can be more than one, if the host block is bigger than the model block. For each of them we pop the samples that came in first into our model block, process them, and push them into the output ring buffer. Whatever is left waits for the next callback."

---

<div class="tag">Solution</div>

## TODO 3: Hand the samples back

```cpp
if (m_output.available() >= num_samples) { // <- fill out
    m_output.pop(samples, num_samples); // <- fill out
} else {
    std::fill_n(samples, num_samples, 0.0f);   // Nothing ready yet: silence
}
```

Note:
- Hand the samples back to the host
- At the start nothing is ready: write silence
- Leave the buffer alone and the host hears its own input

**Script:**

"In the last TODO we hand back the samples to the host. However, we have to deal with the case where there are no samples to hand back, yet, at the beginning of the process. If nothing's ready, we need to write silence. If you leave the buffer alone, the host just
hears its own input."

---

<div class="tag">Done</div>

## The model runs on any host block size

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
- Runs at any host block size
- Open: the output starts with silence, that's latency
- Open: each callback does nothing or a full forward pass
- How long does it take? Can we run it in time? Let's measure

**Script:**

"The model now runs at any host block size. But two things are open. The output starts with silence, which is latency. And each callback either does nothing or a full forward pass. How long does that forward pass take? We need to know about how much latency we have, and whether we can run the model in time. So let's measure it."
