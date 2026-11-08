<h1>Prepare</h1>

Contract, latency, memory

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- Prepare basically checks everything again, now with the contract (the hosts block information), loads the models, allocates everything and starts the pool's threads with the first prepare in the process

---

<div class="tag">The map</div>

## Prepare runs when the host<br><span class="then">announces its blocks.</span>

{{MAP:show=create|focus=prepare}}

Note:
- Prepare typically runs on the main thread as well, in juce you would call prepare in your prepareToPlay callback

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 8</div>

## <code>prepare</code> took two numbers.<br><span class="then">The host's block size, our worst inference time.</span>

```cpp
void prepare(size_t host_block_size, double max_inference_ms) {
    …
    // the worst inference time, rounded up to whole callbacks
    const size_t rounded_max_inference_samples = …;
    // from the two: the prefill and the latency…
    m_prefill = (m_model.m_input_size - host_block_size) + rounded_max_inference_samples;
    m_latency = m_prefill + m_model.m_latency;
    // …and the size of the output buffer
    m_output = ThreadSafeRingBuffer(rounded_max_inference_samples + m_model.m_input_size);
    …
}
```

Note:
- Lets recap the prepare function of part 1, in step 8
- It took two arguments: the host's block size and our max inference time
- From these two we calculated everything else: how many callbacks the worker gets, the prefill, the latency and the size of the output buffer
- The block size came from the host, in prepareToPlay
- But where did the max inference time come from?

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 4</div>

## The max inference time:<br><span class="then">benchmarked by hand.</span>

```cpp
constexpr double k_max_inference_ms = 46.4;  // <- fill out after step 4
```

| step 4 | |
|---|---|
| mean, median, min, p99, max | over many runs, on our machine |
| the max decides | our worst case, on our machine, for ONNX Runtime |

Note:
- It came from step 4: we benchmarked the model by hand
- Many runs, and the max of them decides: our worst case
- And then we copied that number into the code by hand
- But it only holds for our machine and for ONNX Runtime: another machine, another engine or another model, and we benchmark again

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Contract</div>

## <code>prepare</code> takes a contract.<br><span class="then">What the host promises, how to measure.</span>

```json
{ "hard": { "budget": "measured", "warmup": "until_stable", … } }
```

```cpp
m_contract.hard_host_blocks(1, max_block_size, sample_rate);   // the host's side
const anira::PlanReport report = m_handler->prepare(m_contract);
```

<div class="statement-points">
  <div>Host blocks: the smallest and the largest the host may send, and the rate</div>
  <div><code>"budget": "measured"</code>: the max inference time, measured at <code>prepare</code></div>
  <div><code>…</code>: more options, for process, in the next chapter</div>
</div>

Note:
- In anira prepare does not take two numbers, it takes a contract
- Hard is the contract of a stream: blocks in and blocks out, in every callback
- This is the file from step 9. It carries no host blocks: we fill them in from prepareToPlay, the smallest and the largest block the host may send, and the sample rate
- And instead of a max inference time we write "measured": anira measures it at prepare, on the user's machine
- And the dots: the contract has more options, for process, and we come to them in the next chapter
- (A second kind, Async, a job with a deadline instead of a stream, is not in this release: prepare refuses it)

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Budget</div>

## Measured at <code>prepare</code>.<br><span class="then">Step 4, done by the library.</span>

<div class="statement-points">
  <div><code>"until_stable"</code>: the model runs until its last four times settle, at most 32 runs</div>
  <div><code>"measured"</code>: the max of those last four, on the user's machine, with this engine</div>
  <div>Or by hand: <code>{"fixed": n}</code> runs, <code>{"ms": x}</code> as the budget</div>
</div>

<div class="cards-note">Measured once per model and process: every further instance, and every re-prepare, reuses it.</div>

Note:
- So prepare runs the warm-up, and the warm-up is the measurement
- The first runs of a model are slow: the engine allocates, compiles and fills its caches. That happens here and not in the first callback
- until_stable runs the model on the inference threads until its last four times lie within 20 percent of their median, at most 32 runs
- measured takes the max of those last four: step 4, done by the library, on the user's machine, with this engine
- Both can also be given by hand: a fixed number of runs, and a budget in milliseconds, then the latency is the same on every machine
- anira measures a model once per process: a second instance of your plugin, or a re-prepare at another block size, reuses the figure and runs no warm-up. So ten identical plugins report the same latency
- For your own models, anira's examples have a benchmark fixture: real-time factor and underruns per block size and engine

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Report</div>

## <code>prepare</code> reports back.<br><span class="then">What it measured, and whether it keeps up.</span>

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
- And prepare gives us a report back, the plan report: what it measured, how long the warm-up ran, and a verdict, whether the model keeps up on this machine
- The verdict comes from the real-time factor: the measured time over the time one model block of audio lasts. Ours: 12.7 ms over 46.4 ms, 0.27
- Divided by the inferences that can run at once, one for our model with state, that is the load
- Below 0.75 comfortable, up to 1 marginal, from 1 on not real-time. prepare still succeeds then, but misses are certain
- [click] What to do with it is the host's choice: comfortable plays,
- [click] marginal plays with a note, because load the measurement could not see can push it over,
- [click] and not real-time is held, and the user is asked: "Try nonetheless". anira's JUCE and CLAP examples do exactly that
- An offline bounce never runs against the clock, so it is never held
- Our check in step 9 prints this report

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 8</div>

## One model, loaded by hand.<br><span class="then">In prepare, the first time.</span>

```cpp
void prepare(size_t host_block_size, double max_inference_ms) {
    m_worker.stop();
    // The model, loaded the first time: it takes a while and allocates.
    if (!m_engine.loaded()) { m_engine.load(m_model.m_path); }
    …
}
```

Note:
- Before all of that, part 1's prepare loaded the model
- By hand: one engine, one file, the first time prepare is called
- Loading takes a while and allocates, so it belongs in prepare and not in the callback

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Models</div>

## Every model, loaded at <code>prepare</code>.<br><span class="then">Before the warm-up measures it.</span>

<div class="statement-points">
  <div>Every entry this build can run: its model loads, a plan per engine and provider</div>
  <div>Pooled: a second handler with an equal model shares the load</div>
  <div>A model that does not load fails <code>prepare</code>, with the file and the reason</div>
</div>

Note:
- In anira we load nothing ourselves: prepare loads every model of the file
- Every entry this build can run, so a file with two enabled engines loads both, and we can switch between them later
- Pooled per process: a second handler with an equal model shares the load, as we saw in Create
- And only then the warm-up runs, on what was loaded
- A model that does not load fails prepare, and the error names the file and the reason

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 3 and 6</div>

## Every buffer sized by hand.<br><span class="then">From the worst moment we could think of.</span>

```cpp
m_input  = ThreadSafeRingBuffer(m_model.m_input_size);
m_output = ThreadSafeRingBuffer(worker_time + m_model.m_input_size);
```

Note:
- In part 1 we also sized every buffer by hand
- Step 3 by thinking through the worst moment, step 6 from what we knew: one model block in, and the worker's time plus one model block out
- All of it allocated in prepare, never in the callback: a rule that was ours to keep

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Memory</div>

## Allocated at <code>prepare</code>.<br><span class="then">Nothing after.</span>

<div class="statement-points">
  <div>A ring per streamed tensor, sized from the host blocks, the model block and the budget</div>
  <div>Slots: two per inference that can be in flight</div>
  <div>After <code>prepare</code>: <code>process</code>, <code>push</code> and <code>pop</code> allocate nothing</div>
</div>

<div class="cards-note">The host changes its blocks: <code>prepare</code> again. It is the one call that may allocate.</div>

Note:
- anira does all of this for us: it sizes and allocates everything at prepare, and nothing after
- It sizes from what we declared: the host blocks from the contract, the model block from the model file, and the budget it just measured
- A ring per streamed tensor, and slots for the jobs between the audio thread and the pool, two per inference that can be in flight
- The state pairs got their two buffers at create; prepare sets both to zero, a new stream
- After prepare, process, push and pop allocate nothing
- When the host changes its blocks we call prepare again: it is the one call that may allocate

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Steps 7 and 8</div>

## The latency by hand, for 512 only.<br><span class="then">Every other block size: bypassed.</span>

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
- And the latency we calculated by hand in step 7: 1536 for the buffer adaptation, 1024 for the worker, two callbacks for 15.3 ms, and the model's own 2224. 4784 samples
- But only for a host block of 512: the step 8 plugin bypassed every other setting
- A real host calls with other sizes, and even with smaller blocks than it announced

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
- anira calculates the same, at prepare, for the block size the host uses
- In the animation four host blocks of 512 fill one model block of 2048, and only then can the model run. That wait is the block adaptation
- P is the model block, b the host block: P minus the largest block that divides both, 2048 minus 512, so 1536. No block-size adapter can do better
- Then the inference: as many host blocks as the measured budget needs. 12.7 ms do not fit in one block of 11.6 ms, so two: 1024
- Plus the model's own 2224 from the model file: 4784, the number from step 7
- Step 4 measured 15.3 ms, anira's warm-up 12.7 ms: both are two callbacks, so both give 4784
- And it is measured on the user's machine: a slower machine gets a larger latency, not dropouts

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
- And unlike part 1, anira is not limited to one block size
- prepareToPlay announces the largest block, not the only one: loop a region in a DAW and the callback at the loop point is shorter. Logic is known for it
- So we declare 1 up to the announced size, and the latency holds for the worst block in that range
- The block adaptation becomes P − 1 instead of P − gcd(b, P) (Rath & Geier, LAC 2026): with the same 12.7 ms that is 5295, 511 samples more
- And the box: a host that needs one fixed number, the same on every machine and for every block size, declares it in the contract, per output. Below the model's own latency prepare refuses it, and if it is less than the machine needs, the late blocks become misses

---

<div class="tag">anira · Latency</div>

## Ask the handler for it.<br><span class="then">The output is prefilled to match.</span>

```cpp
m_handler->prepare(m_contract);
m_latency = m_handler->get_latency(0);   // output slot 0, audio_out: for the host
```

<div class="statement-points">
  <div><code>get_latency(slot)</code>: one output's latency in its samples, valid from <code>prepare</code> on</div>
  <div>The output starts with zeros: the latency minus the model's own 2224, like step 8's prefill</div>
  <div>From then on every result comes out exactly that late</div>
</div>

Note:
- So what is left for us: after prepare we ask the handler for the latency, and tell the host
- get_latency gives it per output, in samples, here output slot 0, audio_out
- And anira prefills the output for us, as we did by hand in step 8: zeros, the latency minus the model's own delay, and the model's 2224 come on top by themselves
- So the first samples out are those zeros, and from then on every result arrives exactly as late as get_latency says

---

<div class="tag">The map</div>

## After prepare:<br><span class="then">loaded models, a latency, every buffer the stream needs.</span>

{{MAP:show=create,prepare}}

Note:
- After prepare the models are loaded, pooled, and the plans exist
- The latency went to the host, the rings and slots exist, and the pool threads run
- From here on only the audio thread's calls remain: the third row
- In short: the host promises its blocks, and anira answers with a latency, measured, sized and allocated at prepare
- Next: process, one block through all of it
