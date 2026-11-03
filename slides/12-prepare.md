<h1>Prepare</h1>

Contract, latency, memory

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - The second row of the map: what happens when the host announces its blocks. The contract, the latency that follows from it, the memory it sizes, and the warm-up.

---

<div class="tag">The map</div>

## Prepare runs when the host<br><span class="then">announces its blocks.</span>

{{MAP:show=create|focus=prepare}}

Note:
    - The row we fill now: prepareToPlay. The host says its block size and sample rate; the contract carries them into prepare, and get_latency gives the number for setLatencySamples.
    - Still the main thread: prepare may allocate, load and run the model. It is the last call that may.

---

<div class="tag">anira · Contract</div>

## The Hard contract:<br><span class="then">what the host promises, what it gets.</span>

```json
{ "hard": { "budget": { "ms": 8.2 }, "warmup": { "fixed": 3 },
            "on_miss": "zeros", "wait_ratio": 0 } }
```

```cpp
contract.hard_host_blocks(block, block, sample_rate);   // from the host
```

<div class="statement-points">
  <div>Host blocks: the smallest, the largest, the rate; equal min and max earn the tight latency</div>
  <div>Budget: the worst case per inference; warm-up: model runs before the stream starts</div>
  <div><code>on_miss</code> and <code>wait_ratio</code>: what a callback does when a result is late, in Process</div>
</div>

<div class="cards-note">Async, a job with a deadline instead of a stream, follows with M3; alpha.2 refuses it at <code>prepare</code>.</div>

Note:
    - The contract is the scheduling regime of one handler, chosen at prepare and fixed until the next. Hard is a stream: blocks in, blocks out, in every callback.
    - The file carries no host blocks: the plugin patches them from the host and prepares again when the host changes them.
    - A host with variable blocks gives a range; the reported latency then covers the worst case across it.
    - In alpha.2 the budget is explicit and the warm-up a fixed count. "measured" and "until_stable" load from a file, and prepare refuses them until alpha.3.
    - Async is for work that is not a stream: a camera frame, an analysis, a generation. It is designed, not built: M3. Everything in the hands-on is Hard, step 11 included.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 6</div>

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
    - Step 6's number: a full block to collect, a full block for the worker, the model's own delay. Safe at any host block size, and generous: at 512 samples per callback, most of the first 2048 is waiting for nothing.

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
    - Host block b = 512, model block P = 2048, budget 8.2 ms, 44.1 kHz.
    - Adaptation: P − gcd(b, P) = 1536 samples. The minimum any block-size adapter needs (Rath and Geier, LAC 2026): the model cannot run before its block is complete, and the last host block that completes it is the one that waits least.
    - Inference: 8.2 ms is 0.18 model blocks of audio, 0.7 host blocks. One host block of waiting is enough: 512 samples. With wait_ratio β the callback may absorb part of it.
    - Plus the model's own 2224, declared in the model file as "latency": 2224 on the output.
    - 4272 against our 6320: the same guarantee, 2048 samples less. At b = 441: gcd is 1, adaptation 2047, inference 441, total 4712.
    - The formula also says when it cannot work: if the budget exceeds the audio per hop times the threads, the queue grows without bound. alpha.2 does not check that yet; from alpha.3 prepare refuses it with ANIRA_ERROR_BUDGET.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 2 and 5</div>

## Every buffer sized by hand.<br><span class="then">From the worst moment we could think of.</span>

```cpp
m_input  = RingBuffer(max_block_size + k_model.m_input_size);
m_output = RingBuffer(max_block_size + k_queue_capacity * k_model.m_input_size);
m_to_worker = moodycamel::ReaderWriterQueue<ModelBlock>(k_queue_capacity);
```

Note:
    - Step 2 sized the rings by thinking through the worst moment; step 5 added the queues, with a capacity of eight model blocks in flight, a guess.
    - All of it allocated in prepare, none of it in the callback: that rule was ours to keep, and every size was ours to get right.

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
    - What step 2 and step 5 sized by hand, anira sizes from what was declared: the host blocks from the contract, the window and the latency from the model file.
    - The rings hold the stream's own element type, not only float: the contract's ring dtype per streamed slot.
    - Slots are the job records between the audio thread and the pool, preallocated: the audio thread claims a free one and leaves.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 3</div>

## We measured the budget by hand.

| step 3 | |
|---|---|
| mean, p95, p99, max | over many runs, on our machine |
| the max decides | 8.2 ms, the number every step since uses |

Note:
    - A benchmark we wrote, run on one machine. It has to be repeated for every model, every engine, every machine the plugin runs on.

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Budget</div>

## Warm-up at <code>prepare</code>.<br><span class="then">The budget from the contract.</span>

<div class="statement-points">
  <div><code>{"fixed": n}</code>: n model runs at <code>prepare</code>, on the thread that calls it</div>
  <div>In alpha.2 the budget is explicit: the number from step 3</div>
  <div><code>"measured"</code>, the worst time of the warm-up, and <code>"until_stable"</code> follow with alpha.3</div>
  <div>A benchmark fixture for your own model: real-time factor and underruns</div>
</div>

Note:
    - Warm-up blocks prepare, so prepareToPlay takes a few model runs longer; {"fixed": n} bounds it.
    - With alpha.3 the budget can be measured on the machine the plugin runs on: the largest inference time over the warm-up. until_stable runs until the last four of at most 32 runs lie within 20 % of their median.
    - Step 3's fixture is anira's: ProcessBlockFixture over Google Benchmark, parameterised by block size, engine and model. In v3 it lives in the examples, not in the library.

---

<div class="tag">The map</div>

## After prepare:<br><span class="then">a latency, and every buffer the stream needs.</span>

{{MAP:show=create,prepare}}

Note:
    - The prepare row, filled. The latency went to the host; the rings and slots exist; the pool threads run.
    - From here on, only the audio thread's calls: the third row.

---

<div class="tag">Take away</div>

## The host promises its blocks.<br><span class="then">anira answers with a latency.</span>

<div class="statement-points">
  <div>The contract: host blocks, budget, warm-up, and what a miss delivers</div>
  <div>Latency: computed from block size, budget and the model, at <code>prepare</code></div>
  <div>Memory: sized and allocated at <code>prepare</code>, never in the callback</div>
</div>

Note:
    - Next: process, one block through all of it.
