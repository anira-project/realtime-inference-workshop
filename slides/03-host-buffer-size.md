<h1>Step 3<br>Model input size<br>vs. host buffer size</h1>

<!-- .slide: data-state="no-header" -->
<!-- kind: exercise -->

Note:
- let's have a look into the buffer sizes.

**Script:**

*Ask the room: "What buffer sizes do you ship with?" Collect a few
numbers: 64, 128, 256, 441...*
"As we can hear we have a wide range of host block sizes. The model takes 2048 samples, but the host can give us anything. In this part of the workshop our host hands us 512 samples per call, which is not 2048, but at least it divides it. Making it work for any block size is a topic of its own, and we come back to it in part 2."

---

<div class="tag">Motivation</div>

## What is the problem? <span class="then">Two owners, two block sizes.</span>

<div class="streams">
  {{STREAM:host=512|model=2048|length=4096|seconds=7}}
</div>

<div class="cards-note corner-note">Part 1 assumes a constant host block that divides 2048: 512. Every other case comes in part 2.</div>

Note:
- Two owners, two block sizes to deal with
- Model: fixed at export
- Host: depends on device and user, can change while running
- They can match by coincidence, we can't count on it
- Part 1 of the workshop assumes the host block is constant and divides 2048: we chose 512
- Blocks that change while running, or are multiples od 2048: part 2, with anira

**Script:**

"So our problem is, we have two owners, with two different block sizes. The model's block is fixed at export. The host's block comes from the device and the user, and it can even change while running. Let's look at our case: the host hands us 512 samples. In the first three callbacks nothing happens, because we haven't reached the model's block size yet. The fourth callback runs the whole model.
Sometimes the block sizes will match by coincidence, but we cannot count on that happening. And sizes that don't even divide 2048, like 441, make it harder still; we keep those for part 2."

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
- we need a reliable way of passing the samples, that makes sure the model always get a block of 2048 and the host will always get 512 samples
- The approach: A ring buffer on each side of the model
- Host pushes whatever it has into the first ringbuffer
- Full block in: the model runs, writes 2048 to the output ring buffer
- Host takes its block size each call, 512 in this picture

**Script:**

"So we need a reliable way of passing samples from the host to the model that makes sure the model will always get a block of 2048 and the host will always get the amount of samples it needs back in time. The approach is to use ring buffers on each side of the model. The host pushes whatever it has into the first ring buffer. Once a
full model block is in, the model runs and writes 2048 samples to the output
ring buffer. From there the host takes as many samples as its block size asks for at each call, 512 in this picture."

---

<div class="tag">Goal</div>

## The goal<br><span class="then">The model runs on the host's 512-sample blocks.</span>

<div class="statement-points">
  <div><small>1</small>Create the ring buffers</div>
  <div><small>2</small>Pump whole blocks through the model</div>
  <div><small>3</small>Hand the host its samples back</div>
</div>

Note:
- Goal: the model gets 2048, the host gets its 512 back
- Size the ring buffers
- Pump whole blocks through the model
- Hand the samples back to the host

**Script:**

"So, the goal is that the model always gets its 2048 samples and the host always gets its 512 back. For that we need to give the ring buffers a size, pump whole blocks through the model and then hand the samples back to the host."

---

<div class="tag">Given</div>

## What we get<br><span class="then">1 new helper, 1 file to edit.</span>

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
    {{FILE:ring_buffer.h|new}}
  </div>
  <div class="given-column exercise">
    <div class="given-label">Exercise</div>
    <div class="given-folder">03_host_buffer_size/exercise/</div>
    {{FILE:main.cpp}}
  </div>
</div>

Note:
- Same model, signals and engine helper
- New: a ring buffer
- The exercise isn't writing a ring buffer, it's sizing and using it

**Script:**

"We still have the same model and signal files, we also still have the engine helper file. But now we get a new helper file: a ring buffer. The exercise won't be to create a ring buffer yourself, but rather to handle it, so to give it the right size and use it."

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
- A ringbuffer is like a queue for samples with a fixed size
- Allocates once with the correct capacity or size, when created, never again
- `push` at the back, `pop` from the front: first in, first out method
- If the buffer has the wrong size: it throws, so we notice right away

**Script:**

"Let's look at the ring buffer: a queue for samples with a fixed size. It allocates once, when we create it, and never again. We add samples at the back with push and take them from the front with pop, first in, first out. And if we size it wrong, it throws, so we notice right away instead of getting strange audio."


---

<div class="tag">Task: <code>main.cpp</code></div>

## The task

<div class="task-timer" data-minutes="8"></div>

Fill in the TODOs in `steps/03_host_buffer_size/exercise/main.cpp`

```bash
cmake --build --preset release --target step03_exercise
./build/bin/step03_exercise
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
      <div class="run-ok">✓ the host gets the model's stream back</div>
    </div>
  </div>
</div>

Note:
- Three things: size the ring buffers, pump a whole block, hand the samples back
- `main.cpp` in steps/03_host_buffer_size/exercise, build commands on the slide
- 8 minutes, don't worry if you don't finish, we solve it together

**Script:**

"Now it's your turn again. We will now give the ring buffers the proper size. Then we will pump a whole block though our model and finally hand the samples back to the host. You'll find the main.cpp in steps/03_host_buffer_size/exercise. The build commands are on the slide. You get 8 minutes to try and solve it. Again, don't worry if you don't make it. We will go through the solutions together and you can find the solutions in the solutions main.cpp too."
*Timebox. The interesting TODO is the sizing. Let people argue about it.*

---

<div class="tag">Solution</div>

## TODO 1: Give the ring buffers their size<br><span class="then">The fullest moment, callback by callback.</span>


**Synchronous**: the model runs inside the callback, so its block is in <code>m_output</code> before the callback returns.


```cpp
const size_t capacity = k_model.m_input_size; // <- fill out
```


Note:
- Synchronous case: the model runs inside the callback, nothing happens between callbacks
- So one callback is: push 512, maybe run the model, pop 512
- Input: fills 512 per call, full at 2048 in callback 4, emptied in the same call
- Output: the model's 2048 land when the previous block is fully drained (512 divides 2048), so 2048 is the peak
- So one model block for both is enough
- For host sizes that don't divide 2048 or are a multiple of 2048 this isn't true anymore, but we'll get into that in part 2

**Script:**

"Where does the size come from? A ring buffer has to hold the most samples that are ever in it at once, so let's follow the fill level callback by callback. One thing first: this is the synchronous case. The model runs inside the callback, so every callback is: push 512, maybe run the model, pop 512, and when the callback returns, everything is done.

The input buffer grows by 512 per call: 512, 1024, 1536. In callback 4 it reaches 2048, a whole model block, and we take it all out in the same call. So the input never holds more than 2048.

On the output side, the model puts its 2048 in during callback 4, and the host takes 512 right away. Over the next three callbacks the host drains the rest, so when the next block arrives, the output is empty again. Because 512 divides 2048, that lines up exactly. So the peak is 2048 on both sides: one model block.

This only holds because the model runs synchronously and 512 divides 2048. When the model runs on another thread in step 6, or the sizes don't divide in part 2, we need more room."

---

<div class="tag">Solution</div>

## TODO 2: Pump whole blocks

```cpp
m_input.push(samples, num_samples); // <- 1. fill out

const bool has_block = m_input.available() >= k_model.m_input_size; // <- 2. fill out

if (has_block) {
    m_input.pop(m_model_block.data(), k_model.m_input_size); // <- 3. fill out
    m_engine.process(m_model_block.data(), k_model.m_input_size, m_state);
    m_output.push(m_model_block.data(), k_model.m_input_size); // <- 3. fill out
}
```

Note:
- First we push the host's samples into the input ring buffer
- Check if 2048 sample are available -> only pump one model block at a time
- With 512 per call that is every fourth callback, never more than one block
- Then: pop, process, push to the output ring buffer

**Script:**

"We should only pump the block when we have at least 2048 samples. So first we push the host's samples into the input ring buffer, and then we check whether a whole model block is waiting: available at least 2048. With 512 samples per call that's true every fourth callback, and it's never more than one block. Then we pop the samples that came in first into our model block, process them, and push them into the output ring buffer. Whatever is left waits for the next callback."

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
- Hand the samples back to the host if enough samples (512) are available in the output buffer
- At the start nothing is ready: write silence

**Script:**

"In the last TODO we hand back the samples to the host. However, we have to deal with the case where there are no samples to hand back, yet, at the beginning of the process. If nothing's ready, we need to write silence. If you leave the buffer alone, the host just
hears its own input."

---

<div class="tag">Bonus question</div>

## How late is the host's output?

<div class="lat-add">
  <div class="lat-term fragment"><span>buffer adaptation</span><small>2048 − 512</small><strong>1536</strong></div>
  <div class="lat-op fragment">+</div>
  <div class="lat-term fragment"><span>the model itself</span><small>in this case?</small><strong>2224</strong></div>
  <div class="lat-op fragment">=</div>
  <div class="lat-term total fragment"><span>latency</span><small>samples</small><strong>3760</strong></div>
</div>

<div class="cards-note corner-note fragment">For 512 into 2048. Other host block sizes give other numbers: part 2.</div>

Note:
- Ask first: How long does it take to hear our first processed samples?
- The model needs 2048, the host brings 512 at a time
- Callbacks 1 to 3: `m_input` collects 512, 1024, 1536 samples, `m_output` is empty, so the host gets silence: 3 × 512 = 1536 
- Callback 4: the block is complete, the model runs inside the callback
- The first 3 callback make up the Buffer adaptation, the first part of our latency.
- In general: model block minus the largest block that divides both, here 2048 − 512
- Plus the model itself. Here 2224: 1696 from the decoder's transposed convolutions, 528 from the PQMF filter bank (from the export script, not visible from C++)
- Total 3760 samples, about 85 ms at 44.1 kHz
- Only for 512: other host block sizes give other numbers, part 2

**Script:**

*Ask the room: "The host got silence at the start. How late is what it hears?"*
"First, the buffer adaptation. The model needs 2048 samples, but the host brings them 512 at a time. In the first three callbacks we only collect, and there's nothing to give back yet, so the host gets silence: three times 512, 1536 samples. In the fourth callback the block is complete, the model runs, and the first 512 samples of its output go out right away. So sample 0 went in at the start of the first callback and comes out at the start of the fourth: 1536 samples later. And from then on the output buffer always holds what the next three callbacks need, so every sample the host hears went in 1536 samples earlier: 2048 minus 512.
But that's not all. The model itself is late too. *Ask: "How late, in this case?"* For our model it's 2224 samples: 1696 from the transposed convolutions in the decoder, and 528 from the PQMF filter bank. We can't see that from C++, it comes from the export script.
So in total the host hears our output 3760 samples late, about 85 milliseconds. And this number is only right for 512: other host block sizes give other numbers, and part 2 shows how."

---

<div class="tag">Done</div>

## The model runs on the host's 512-sample blocks <span class="then">But is it real-time capable?</span>

<div class="congrats">
  <svg viewBox="0 0 120 120" aria-hidden="true">
    <circle cx="60" cy="60" r="52"/>
    <path d="M36 62 L53 78 L85 44"/>
  </svg>
</div>

Note:
- Ou model now runs on the host's 512-sample blocks, and we know how late
- But we now have three callbacks doing nothing, the fourth a full forward pass
- To know if we are even realtime capable we need to know how long our forward pass takes
- Can we run it in time? Let's measure

**Script:**

"The model now runs on the host's 512-sample blocks, and we know how late the output is. But one thing is open: three callbacks do nothing, and the fourth runs a full forward pass. How long does that forward pass take, and can we run it in time? So let's measure it."
