<h1>Prepare</h1>

Contract, latency, memory

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- Prepare: the second row of the map.
- This is what happens when the host announces its blocks.
- Prepare checks everything again, now with the contract, loads the models, pooled, and runs their warm-up as they load, builds the plans, and starts the pool's threads with the first prepare in the process.
- Then the rest: the latency, the rings and the slots for the jobs in flight.
- We look at the contract, the latency that follows from it, the memory it sizes, and the warm-up.

---

<div class="tag">The map</div>

## Prepare runs when the host<br><span class="then">announces its blocks.</span>

{{MAP:show=create|focus=prepare}}

Note:
- Prepare runs when the host announces its blocks.
- The row we fill now is prepareToPlay.
- The host tells us its block size and sample rate, and the contract carries them into prepare.
- get_latency then gives us the number for setLatencySamples.
- We are still on the main thread: prepare may allocate, load and run the model, and it is the last call that may.

---

<div class="tag">anira · Contract</div>

## The Hard contract:<br><span class="then">what the host promises, what it gets.</span>

```json
{ "hard": { "budget": "measured", "warmup": "until_stable",
            "on_miss": "bypass", "wait_ratio": 0 } }
```

```cpp
contract.hard_host_blocks(1, max_block_size, sample_rate);   // from prepareToPlay
```

<div class="statement-points">
  <div>Host blocks: the smallest and the largest the host may send, and the rate</div>
  <div>Budget: the worst case per inference; warm-up: model runs before the stream starts</div>
  <div><code>on_miss</code> and <code>wait_ratio</code>: what a callback does when a result is late, and how long it may wait for one, in Process</div>
</div>

<div class="cards-note">Async, a job with a deadline instead of a stream, follows in a later release; today <code>prepare</code> refuses it.</div>

Note:
- The Hard contract: what the host promises, and what it gets.
- A contract says how one handler is scheduled; it is fixed at prepare and holds until the next prepare.
- Hard is a stream: blocks in and blocks out, in every callback.
- The file carries no host blocks: the plugin fills them in from the host, and prepares again when the host changes them.
- Host blocks are the smallest and the largest the host may send, and the sample rate.
- The block size from prepareToPlay is the largest, not the only one: we declare 1 up to it, and the latency covers the worst case.
- The budget is the worst time of one inference, measured at prepare on the user's machine; the warm-up runs the model until its times are stable, before the stream starts.
- on_miss and wait_ratio say what happens when a result is late; our file asks for bypass, the input block in place of the late one, and we come to the choices in Process.
- wait_ratio also enters the latency: a callback that may wait part of a block needs less latency.
- Async is not in this release, so prepare refuses it.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 7</div>

## The minimal latency, for 512.<br><span class="then">And for every other block size?</span>

<div class="lat-sum">
  <div class="lat-part">buffer adaptation<small>2048 − 512</small><strong>1536</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">rounded max inference time<small>15.3 ms → 2 callbacks</small><strong>1024</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">the model itself<strong>2224</strong></div>
  <div class="lat-plus">=</div>
  <div class="lat-part total">latency<strong>4784</strong></div>
</div>

Note:
- In step 7 we worked out the minimal latency by hand: 1536 for the buffer adaptation, 1024 for the worker, two callbacks for 15.3 ms, and the model's own 2224. 4784 samples.
- But only for a host block of 512, the one size the whole first part used.
- A host can call with other sizes, and with smaller blocks than it announced.
- Now let's see what anira computes, for the block size the host actually uses.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Latency</div>

## Computed at <code>prepare</code>,<br><span class="then">for the block size the host uses.</span>

{{STREAM:host=512|model=2048|length=4096|seconds=5}}

<div class="lat-sum">
  <div class="lat-part">block adaptation<small>P − gcd(b, P)</small><strong>1536</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">inference<small>⌈budget ÷ host block⌉ host blocks</small><strong>512</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">the model itself<strong>2224</strong></div>
  <div class="lat-plus">=</div>
  <div class="lat-part total">latency<strong>4272</strong></div>
</div>

<div class="lat-why">anira gives the inference ⌈budget ÷ host block⌉ host blocks: 8.2 ms fit in one of 11.6 ms.</div>

Note:
- anira computes the latency at prepare, for the block size the host actually uses.
- Our example: the host delivers 512 samples, the model wants 2048, at 44.1 kHz, and say the measured budget is 8.2 ms.
- In the animation, four host blocks fill one model block, and only then can the model run.
- That wait is the block adaptation.
- On the slide, P is the model block and b the host block, and the adaptation is P minus the largest block that divides both: 2048 minus 512, so 1536.
- No block-size adapter can do better, because the model cannot start before its block is complete.
- For the inference, by hand we waited a whole model block; anira waits as many host blocks as the budget needs.
- 8.2 ms fit in one host block of 11.6 ms, so that is 512 samples.
- Add the model's own 2224 from the model file: 4272 in total.
- The budget is measured on the user's machine, so the latency is that machine's too: a slower machine gets a larger number, not dropouts.

---

<div class="tag">anira · Latency</div>

## Smaller blocks than announced.<br><span class="then">The latency covers the worst one.</span>

```cpp
contract.hard_host_blocks(1, max_block_size, sample_rate);   // 1 up to the announced size
```

| Host blocks | Latency |
|---|---|
| exactly 512, every callback | 4272 |
| any block from 1 to 512: step 9 | 6320 |

Note:
- prepareToPlay announces a block size, but that is the largest the host sends, not the only one.
- Loop a region in a DAW, and the callback at the loop point is shorter.
- Logic is known for it: what prepare announces and what process delivers do not always match.
- So we declare 1 up to the announced size, and anira's latency has to hold for the worst block in that range.
- The block adaptation alone becomes P − 1 instead of P − gcd(b, P) (Rath & Geier, LAC 2026).
- Step 9 declares 1 to the host's maximum and reports 6320, for every budget up to 37 ms. The 4272 of the previous slide only holds if every block is exactly 512.
- One more rule behind both numbers: our model has two outputs, audio_out and state_out, and anira rounds the latency of such a model up to whole model blocks. 6320 is two blocks, 4096, plus the model's 2224.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 3 and 6</div>

## Every buffer sized by hand.<br><span class="then">From the worst moment we could think of.</span>

```cpp
m_input  = ThreadSafeRingBuffer(k_model.m_input_size);
m_output = ThreadSafeRingBuffer(rounded_max_inference_time + k_model.m_input_size);
```

Note:
- In step 3 we sized the ring buffers by hand, by thinking through the worst moment.
- Step 6 sized them from what we knew: one model block in, and the worker's time plus one block out.
- All of it was allocated in prepare, none of it in the callback.
- That rule was ours to keep, and every size was ours to get right.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Memory</div>

## Allocated at <code>prepare</code>.<br><span class="then">Nothing after.</span>

<div class="statement-points">
  <div>A ring per streamed tensor, sized from the host blocks and the window</div>
  <div>Slots: two per inference that can be in flight</div>
  <div>After <code>prepare</code>: <code>process</code>, <code>push</code> and <code>pop</code> allocate nothing</div>
</div>

<div class="cards-note">The host changes its blocks: <code>prepare</code> again. It is the one call that may allocate.</div>

Note:
- anira allocates at prepare, and nothing after.
- What we sized by hand, anira sizes from what was declared: the host blocks from the contract, the window and the latency from the model file.
- There is a ring per streamed tensor, float32 unless the contract names another dtype for it.
- Slots are the job records between the audio thread and the pool: two per inference that can be in flight, all preallocated.
- The audio thread claims a free slot and leaves.
- The state pairs got their two buffers at create; prepare sets both to zero, a new stream.
- After prepare, process, push and pop allocate nothing.
- When the host changes its blocks, we call prepare again: it is the one call that may allocate.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 4</div>

## We measured the budget by hand.

| step 4 | |
|---|---|
| mean, median, min, p99, max | over many runs, on our machine |
| the max decides | our worst case, on our machine, for ONNX Runtime |

Note:
- In step 4 we measured the budget by hand.
- Mean, median, min, p99 and max, over many runs, on our machine.
- The max decides: our worst case, on our machine, for ONNX Runtime.
- But it was a benchmark we ran on one machine, for one engine.
- It has to be repeated for every model, every engine, every machine the plugin runs on.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Budget</div>

## Warm-up at <code>prepare</code>.<br><span class="then">The budget from the contract.</span>

<div class="statement-points">
  <div><code>"until_stable"</code>: model runs at <code>prepare</code> until their times settle</div>
  <div><code>"measured"</code>: the budget, measured at <code>prepare</code> on the user's machine</div>
  <div>Or explicit: <code>{"fixed": n}</code> runs, <code>{"ms": x}</code> as the budget</div>
  <div>A benchmark fixture for your own model: real-time factor and underruns</div>
</div>

Note:
- anira runs the warm-up at prepare and takes the budget from the contract.
- The first runs of a model are slow: the engine allocates, compiles and fills its caches, and that must not happen in the first callback.
- With until_stable, prepare runs the model on the thread that calls it until the last runs agree, so prepareToPlay takes those runs longer.
- With measured, anira benchmarks the model at prepare, on the user's machine, with this engine: step 4, done by the library.
- Both can also be given by hand: a fixed number of runs, and a budget in milliseconds.
- To measure your own model, the fixture behind step 4 is part of anira: it reports the real-time factor and underruns per block size and engine.

---

<div class="tag">The map</div>

## After prepare:<br><span class="then">loaded models, a latency, every buffer the stream needs.</span>

{{MAP:show=create,prepare}}

Note:
- After prepare the models are loaded, pooled, and the plans exist.
- The latency went to the host, the rings and slots exist, and the pool threads run.
- From here on, only the audio thread's calls remain: the third row.

---

<div class="tag">Take away</div>

## The host promises its blocks.<br><span class="then">anira answers with a latency.</span>

<div class="statement-points">
  <div>The contract: host blocks, budget, warm-up, and what a miss delivers</div>
  <div>Latency: computed from block size, budget and the model, at <code>prepare</code></div>
  <div>Memory: sized and allocated at <code>prepare</code>, never in the callback</div>
</div>

Note:
- The host promises its blocks, and anira answers with a latency.
- The contract holds the host blocks, the budget, the warm-up, and what a miss delivers.
- The latency is computed at prepare, from the block size, the budget and the model.
- The memory is sized and allocated at prepare, never in the callback.
- Next: process, one block through all of it.
