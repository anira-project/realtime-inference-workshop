<h1>Prepare</h1>

Contract, latency, memory

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- Prepare: the second row of the map.
- This is what happens when the host announces its blocks.
- Prepare checks everything again, now with the contract, loads the models, pooled, builds the plans, starts the pool's threads with the first prepare in the process, and then runs the warm-up for every plan.
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
  <div class="lat-part">the worker's time<small>15.3 ms → 2 callbacks</small><strong>1024</strong></div>
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
  <div class="lat-part">inference<small>⌈budget ÷ host block⌉ host blocks</small><strong>1024</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">the model itself<strong>2224</strong></div>
  <div class="lat-plus">=</div>
  <div class="lat-part total">latency<strong>4784</strong></div>
</div>

<div class="lat-why">anira gives the inference ⌈budget ÷ host block⌉ host blocks: 12.7 ms, measured at prepare, take two of 11.6 ms.</div>

Note:
- anira computes the latency at prepare, for the block size the host actually uses.
- Our example: the host delivers 512 samples, the model wants 2048, at 44.1 kHz, and prepare measured 12.7 ms on my machine.
- In the animation, four host blocks fill one model block, and only then can the model run.
- That wait is the block adaptation.
- On the slide, P is the model block and b the host block, and the adaptation is P minus the largest block that divides both: 2048 minus 512, so 1536.
- No block-size adapter can do better, because the model cannot start before its block is complete.
- For the inference, anira does what we did by hand in step 7: it waits as many host blocks as the budget needs.
- 12.7 ms do not fit in one host block of 11.6 ms, they take two: 1024 samples.
- In step 4 we took the max over every timed pass, the first ones included: 15.3 ms. anira takes the max of the last four warm-up runs, once they are stable: 12.7 ms. Both are two callbacks, 1024 samples, so both give 4784.
- Add the model's own 2224 from the model file: 4784 in total, the number we worked out by hand in step 7, now computed by anira.
- The budget is measured on the user's machine, so the latency is that machine's too: a slower machine gets a larger number, not dropouts.

---

<div class="tag">anira · Latency</div>

## Smaller blocks than announced.<br><span class="then">The latency covers the worst one.</span>

```cpp
contract.hard_host_blocks(1, max_block_size, sample_rate);   // 1 up to the announced size
```

| Host blocks | Latency |
|---|---|
| exactly 512, every callback | 4784 |
| any block from 1 to 512: step 9 | 5295 |

<div class="lat-why">For the measured max inference time of 12.7 ms, as on the previous slide.</div>

<div class="note-box">One fixed latency everywhere: <code>"latencies": {"audio_out": 6000}</code></div>

Note:
- prepareToPlay announces a block size, but that is the largest the host sends, not the only one.
- Loop a region in a DAW, and the callback at the loop point is shorter.
- Logic is known for it: what prepare announces and what process delivers do not always match.
- So we declare 1 up to the announced size, and anira's latency has to hold for the worst block in that range.
- The block adaptation alone becomes P − 1 instead of P − gcd(b, P) (Rath & Geier, LAC 2026).
- Step 9 declares 1 to the host's maximum: with the same 12.7 ms that is 5295, 511 samples more, the adaptation's P − 1 instead of P − gcd. The 4784 of the previous slide only holds if every block is exactly 512.
- Both are my machine's: a slower machine measures more and reports a larger latency, not dropouts. The check prints yours.
- And the box: if your host needs one fixed number, the same on every machine and for every block size, you declare it in the contract, per output. It replaces the computed figure, and anira primes the output with it. Below the model's own latency it cannot be delivered, so prepare refuses it. If it is less than the machine needs, the late blocks become misses, filled by the miss policy.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 3 and 6</div>

## Every buffer sized by hand.<br><span class="then">From the worst moment we could think of.</span>

```cpp
m_input  = ThreadSafeRingBuffer(m_model.m_input_size);
m_output = ThreadSafeRingBuffer(worker_time + m_model.m_input_size);
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
  <div>A ring per streamed tensor, sized from the host blocks and the model block</div>
  <div>Slots: two per inference that can be in flight</div>
  <div>After <code>prepare</code>: <code>process</code>, <code>push</code> and <code>pop</code> allocate nothing</div>
</div>

<div class="cards-note">The host changes its blocks: <code>prepare</code> again. It is the one call that may allocate.</div>

Note:
- anira allocates at prepare, and nothing after.
- What we sized by hand, anira sizes from what was declared: the host blocks from the contract, the model block and the latency from the model file.
- There is a ring per streamed tensor, in the tensor's own dtype unless the contract names another: an int16 model takes int16 blocks.
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

<div class="cards-note">Measured once per model: every further instance, and every re-prepare, reuses it. Identical plugins report identical latency.</div>

Note:
- anira runs the warm-up at prepare and takes the budget from the contract.
- The first runs of a model are slow: the engine allocates, compiles and fills its caches, and that must not happen in the first callback.
- With until_stable, prepare runs the model on the inference threads until the last four runs agree, at most 32; prepareToPlay waits for them, so it takes those runs longer.
- With measured, anira benchmarks the model at prepare, on the user's machine, with this engine: step 4, done by the library.
- Both can also be given by hand: a fixed number of runs, and a budget in milliseconds.
- anira measures a model once per process: the second instance of your plugin, or a re-prepare at another block size, reuses the figure and runs no warm-up. So ten identical plugins report the same latency, and prepareToPlay is quick the second time.
- To measure your own model, anira's examples have a benchmark fixture: it reports the real-time factor and underruns per block size and engine. You copy it into your project; the library itself does not link Google Benchmark.

---

<div class="tag">anira · Real-time verdict</div>

## <code>prepare</code> says whether it keeps up.<br><span class="then">The host decides what to do.</span>

<div class="rtv">
  <div class="rtv-scale">
    <div class="rtv-zone ok"><b>comfortable</b><small>load below 0.75</small></div>
    <div class="rtv-zone marginal"><b>marginal</b><small>0.75 to 1</small></div>
    <div class="rtv-zone not"><b>not real-time</b><small>1 or more</small></div>
    <div class="rtv-mark" style="--x: 0.27"><span>our model, my machine<br><code>12.7 ms ÷ 46.4 ms = 0.27</code></span></div>
  </div>
  <div class="rtv-actions">
    <div class="fragment"><b>comfortable</b>play</div>
    <div class="fragment"><b>marginal</b>play, with a note: may drop out under load</div>
    <div class="fragment"><b>not real-time</b>hold it, ask the user: “Try nonetheless”</div>
  </div>
</div>

<div class="cards-note">An offline bounce does not run against the clock: never hold it.</div>

Note:
- prepare does not only measure, it tells you whether the model keeps up on this machine.
- The real-time factor: the measured inference time over the time one model block of audio lasts. Ours: 12.7 ms over 46.4 ms, 0.27. Divided by the inferences that can run at once, one for our stateful model, that is the load.
- Below 0.75 comfortable, up to 1 marginal, from 1 on not real-time: then misses are certain, and the miss policy fills them. prepare still succeeds.
- [click] What the host does is its choice, anira only reports it: a comfortable plan plays.
- [click] A marginal one plays with a note, because load the measurement could not see can push it over.
- [click] A plan that is not real-time is held, bypassed or silent, and the user is asked: "Try nonetheless". anira's JUCE and CLAP examples do exactly that.
- An offline bounce never runs against the clock, so it is never held.
- The verdict is in the plan report that prepare returns, next to the measured time our checks print.

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
