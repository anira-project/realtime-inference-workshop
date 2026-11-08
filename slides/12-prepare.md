<h1>Prepare</h1>

Contract, latency, memory

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- Prepare basically checks everything again, now with the contract (the host's block information), loads the models, allocates everything and starts the pool's threads with the first prepare in the process

---

<div class="tag">The map</div>

## Prepare runs when the host<br><span class="then">announces its blocks.</span>

{{MAP:show=create|focus=prepare}}

Note:
- Prepare typically runs on the main thread as well, in JUCE you would call prepare in your prepareToPlay callback

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
- Let's recap the prepare function of part 1, in step 8
- It took two arguments: the host's block size and our max inference time
- From these two we calculated everything else: the prefill, the latency and the size of our buffer
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
- It came from step 4: we benchmarked the model by hand and then we copied that number into the header
- But it only holds for our machine and for ONNX Runtime: another machine, another engine or another model, and we benchmark again

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Contract</div>

## <code>prepare</code> takes a contract.<br><span class="then">What the host promises, how to measure.</span>

```json
{ "hard": { "budget": "measured", "warmup": "until_stable", … } }
```

```cpp
m_contract.hard_host_blocks(min_block_size, max_block_size, sample_rate);   // the host's side
const anira::PlanReport report = m_handler->prepare(m_contract);
```

<div class="statement-points">
  <div>Host blocks: the smallest and the largest the host may send, and the rate</div>
  <div><code>"budget": "measured"</code>: the max inference time, measured at <code>prepare</code></div>
  <div><code>…</code>: more options, for process, in the next chapter</div>
</div>

Note:
- In anira prepare does not take two numbers, it takes a contract
- The contract is the declaration of the stream
- This is an example JSON contract file for a hard contract.
- A hard contract is for a real-time stream
- Instead of a max inference time the budget can be defined as "measured"
- This means anira will measure the inference time at prepare, on the user's machine
- For how many iterations? The iterations in prepare are defined with the warmup key.
- We have seen that our model took longer in the first inference, which is typical for inference engines.
- This is why we offer the option until stable - until_stable runs the model on the inference threads until its last four times lie within 20 percent of their median, at most 32 runs
- And the dots: the contract has more options, for process, and we come to them in the next chapter
- But the JSON contract file carries no host blocks: we fill them at runtime in prepareToPlay
- In anira the block size does not need to be fixed, it can be a range from minimum to maximum block size
- Soon we will also provide a second contract kind, Async for jobs with a deadline instead of a stream

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
</div>

<div class="cards-note">An offline bounce does not run against the clock: never hold it.</div>

Note:
- And then prepare reports back to us
- It tells us whether on average the model can keep up with our real-time stream
- And what's its real-time threshold
- Remember the real-time threshold in the first part was 1 when our model inference time took 46.4 ms
- The anira library user can then decide what to do with this information.
- Display a warning or bypass the inference

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Prepare</div>

## What else <code>prepare</code> does.<br><span class="then">Load the models, allocate the buffers, start the pool.</span>

<div class="statement-points two">
  <div>Loads every model this build can run, pooled per process</div>
  <div>Sizes and allocates every buffer: a ring per streamed tensor, slots for the jobs in flight</div>
  <div>Starts the thread pool with the first <code>prepare</code> in the process; it runs until the last handler is gone</div>
  <div>Nothing after: <code>process</code> allocates nothing; new host blocks, <code>prepare</code> again</div>
</div>

<div class="cards-note">Part 1 did all of it by hand: one model loaded the first time, two rings sized from the worst moment we could think of, a worker thread started per plugin instance.</div>

Note:
- What else does prepare do for us in anira automatically
- It loads all models according to the way we have discussed in create
- It sizes and allocates the ring buffers, the slots for the jobs in flight to a minimal size
- So that then nothing after allocates, so that our process function is real-time safe
- But what about the overall latency now?

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
- Remember in step 7 we calculated the latency by hand
- And even for a single buffer size this calculation was not that easy...

---

<!-- .slide: class="answer" -->

<div class="tag">anira · Latency</div>

## Computed at <code>prepare</code>,<br><span class="then">for the block size the host uses.</span>

{{STREAM:host=512,768,441|model=2048|length=6144|seconds=7}}

<div class="lat-grid">
  <span></span>
  <div class="lat-part">block adaptation<small>P − gcd(b, P)</small></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">inference<small>⌈budget ÷ host block⌉ host blocks</small></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">the model itself</div>
  <div class="lat-plus">=</div>
  <div class="lat-part total">latency</div>
  <span class="lat-host">host <b>512</b></span><strong>1536</strong><span></span><strong>⌈1.09⌉ × 512 = 1024</strong><span></span><strong>2224</strong><span></span><strong class="total">4784</strong>
  <span class="lat-host">host <b>768</b></span><strong>1792</strong><span></span><strong>⌈0.73⌉ × 768 = 768</strong><span></span><strong>2224</strong><span></span><strong class="total">4784</strong>
  <span class="lat-host">host <b>441</b></span><strong>2047</strong><span></span><strong>⌈1.27⌉ × 441 = 882</strong><span></span><strong>2224</strong><span></span><strong class="total">5153</strong>
</div>

<div class="lat-why">The budget: 12.7 ms, measured at prepare. Host blocks of 11.6 ms (512), 17.4 ms (768) and 10 ms (441).</div>

Note:
- But in a real host the host buffer size can take multiple values
- So we need a general formula, which is displayed here

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


Note:
- And if the host block size is variable, we need to take this into account as well.
- This is why in the anira contract you can define a minimum and maximum block size, this means all block sizes in between are allowed. (e.g. if the DAW loops, block sizes can be smaller.)
- Typically this raises the minimum possible latency.
- In this example from 4784 for a fixed 512 block size to 5295 for the case where all block sizes up to 512 are allowed

---

<div class="tag">anira · Latency</div>

## Ask the handler for it.<br><span class="then">The output is prefilled to match.</span>

```cpp
m_handler->prepare(m_contract);
m_latency = m_handler->get_latency();   // the first output, audio_out: for the host
```

<div class="statement-points">
  <div><code>get_latency()</code>: the first output's latency in samples, valid from <code>prepare</code> on</div>
  <div>The output starts with zeros: the latency minus the model's own 2224, like step 8's prefill</div>
  <div>From then on every result comes out exactly that late</div>
</div>

Note:
- After prepare the output ring buffer is automatically prefilled with zeros
- And we can ask for the latency to compensate, e.g. our dry / wet mixer

---

<div class="tag">The map</div>

## After prepare:<br><span class="then">loaded models, a latency, every buffer the stream needs.</span>

{{MAP:show=create,prepare}}

Note:
- So now all models are loaded and the thread pool has started
- The rings and slots are allocated, and the latency is reported
- From here on only the audio thread's calls remain
