<h1>Prepare</h1>

Contract, latency, memory

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- Prepare: the second row of the map.
- This is what happens when the host announces its blocks.
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
contract.hard_host_blocks(block, block, sample_rate);   // from the host
```

<div class="statement-points">
  <div>Host blocks: the smallest, the largest, the rate; equal min and max earn the tight latency</div>
  <div>Budget: the worst case per inference; warm-up: model runs before the stream starts</div>
  <div><code>on_miss</code> and <code>wait_ratio</code>: what a callback does when a result is late, in Process</div>
</div>

<div class="cards-note">Async, a job with a deadline instead of a stream, follows in a later release; today <code>prepare</code> refuses it.</div>

Note:
- The Hard contract: what the host promises, and what it gets.
- A contract says how one handler is scheduled; it is fixed at prepare and holds until the next prepare.
- Hard is a stream: blocks in and blocks out, in every callback.
- The file carries no host blocks: the plugin fills them in from the host, and prepares again when the host changes them.
- Host blocks are the smallest, the largest and the sample rate.
- Equal min and max give the tightest latency; a range gives a latency that covers its worst case.
- The budget is the worst time of one inference, measured at prepare on the user's machine; the warm-up runs the model until its times are stable, before the stream starts.
- on_miss and wait_ratio say what happens when a result is late; our file asks for bypass, the input block in place of the late one, and we come to the choices in Process.
- Async is not in this release, so prepare refuses it.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 7</div>

## One latency for every block size.<br><span class="then">The same number, whatever block size the host uses.</span>

<div class="lat-sum">
  <div class="lat-part">collect a block<strong>2048</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">hand it over<strong>2048</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">the model itself<strong>2224</strong></div>
  <div class="lat-plus">=</div>
  <div class="lat-part total">latency<strong>6320</strong></div>
</div>

Note:
- Step 7 gave us one latency for every block size: the same number, whatever block size the host uses.
- A full block to collect, 2048, a full block for the worker, 2048, and the model's own 2224: 6320 samples.
- That number is safe at any host block size.
- But it is generous: at 512 samples per callback, most of the first 2048 is waiting for nothing.
- Now let's see what anira computes instead.

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

<div class="lat-why">6320 by hand: we gave the worker a whole model block. anira gives it ⌈budget⌉ host blocks: 8.2 ms fit in one of 11.6 ms.</div>

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
- That is 2048 samples less than our 6320, with the same guarantee.
- The budget is measured on the user's machine, so the latency is that machine's too: a slower machine gets a larger number, not dropouts.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 3 and 6</div>

## Every buffer sized by hand.<br><span class="then">From the worst moment we could think of.</span>

```cpp
m_input  = ThreadSafeRingBuffer(max_block_size + k_blocks_in_flight * k_model.m_input_size);
m_output = ThreadSafeRingBuffer(max_block_size + k_blocks_in_flight * k_model.m_input_size);
```

Note:
- In step 3 we sized the ring buffers by hand, by thinking through the worst moment.
- Step 6 sized them for eight model blocks in flight: a guess.
- All of it was allocated in prepare, none of it in the callback.
- That rule was ours to keep, and every size was ours to get right.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Memory</div>

## Allocated at <code>prepare</code>.<br><span class="then">Nothing after.</span>

<div class="statement-points">
  <div>A ring per streamed tensor, sized from the host blocks and the window, in the tensor's dtype</div>
  <div>Slots: two per inference that can be in flight</div>
  <div>State pairs: two buffers each, swapped, never copied</div>
  <div>After <code>prepare</code>: <code>process</code>, <code>push</code> and <code>pop</code> allocate nothing</div>
</div>

<div class="cards-note">The host changes its blocks: <code>prepare</code> again. It is the one call that may allocate.</div>

Note:
- anira allocates at prepare, and nothing after.
- What we sized by hand, anira sizes from what was declared: the host blocks from the contract, the window and the latency from the model file.
- There is a ring per streamed tensor, in the tensor's own dtype, not only float.
- Slots are the job records between the audio thread and the pool: two per inference that can be in flight, all preallocated.
- The audio thread claims a free slot and leaves.
- The state pairs get their two buffers here.
- After prepare, process, push and pop allocate nothing.
- When the host changes its blocks, we call prepare again: it is the one call that may allocate.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 4</div>

## We measured the budget by hand.

| step 4 | |
|---|---|
| mean, p95, p99, max | over many runs, on our machine |
| the max decides | about 12 ms after the warm-up, on our machine, for LibTorch |

Note:
- In step 4 we measured the budget by hand.
- Mean, p95, p99 and max, over many runs, on our machine.
- The max decides: about 12 milliseconds after the warm-up, for the LibTorch model.
- But it was a benchmark we ran on one machine, for one engine: the ONNX model of part 2 we never measured.
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

## After prepare:<br><span class="then">a latency, and every buffer the stream needs.</span>

{{MAP:show=create,prepare}}

Note:
- After prepare we have a latency, and every buffer the stream needs.
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
