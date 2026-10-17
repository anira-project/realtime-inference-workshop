<h1>The whole picture</h1>

Where the pieces sit

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - One diagram, built up in the order a call travels. Then the plugin.

---

<div class="tag">Architecture</div>

## From the callback to the engine<br><span class="then">and back.</span>

<div class="arch">
  <div class="arch-row host">
    <div class="arch-box"><div class="arch-title">host</div><code>prepare</code> · <code>process</code> · <code>get_latency</code></div>
  </div>
  <div class="arch-row fragment">
    <div class="arch-box handler"><div class="arch-title">handler</div><small>one per stream: rings, slots, the latency</small></div>
  </div>
  <div class="arch-row pipeline fragment">
    <div class="arch-box stage"><div class="arch-title">stage</div><small>pre · post</small></div>
    <div class="arch-box inference"><div class="arch-title">inference</div><small>plans: engine × provider</small></div>
    <div class="arch-box stage"><div class="arch-title">stage</div><small>before · after</small></div>
  </div>
  <div class="arch-row fragment">
    <div class="arch-box context"><div class="arch-title">context</div><small>one per process: the queue, the pool, loaded models</small></div>
  </div>
  <div class="arch-row engines fragment">
    <div class="arch-box engine">LibTorch</div>
    <div class="arch-box engine">ONNX Runtime</div>
    <div class="arch-box engine">LiteRT</div>
    <div class="arch-box engine">ExecuTorch</div>
    <div class="arch-box engine">custom</div>
  </div>
</div>

Note:
    - Host: three calls, the ones from step 8.
    - Handler: one per stream. It owns the rings (step 2), the inference slots (step 6's queues), and computes the latency (step 7) at prepare.
    - Pipeline: the stage around the inference. Pre and post on the audio thread, before and after on the pool.
    - Context: one per process. The job queue, the thread pool, and the loaded models, shared by every handler.
    - Engines: the plans the inference stage can run on. Prebuilt, or yours.
    - Underneath, one C ABI: every box above is a C handle, and the C++ in the examples is a header-only wrapper over it.

---

<div class="tag">In a plugin</div>

## Three places, three calls.

**`prepareToPlay`** — geometry into the contract

```cpp
contract.hard_geometry(block, block, sample_rate);
handler.prepare(contract);
setLatencySamples(static_cast<int>(handler.get_latency()));
```

**`processBlock`** — one tensor over the channel pointers, in place

```cpp
const int64_t shape[2] = {buffer.getNumChannels(), buffer.getNumSamples()};
anira_tensor_init_host_planar(&block, buffer.getArrayOfWritePointers(),
                              buffer.getNumChannels(), ANIRA_DTYPE_F32, 2, shape);
handler.process(block, 0, block, 0);
```

**`releaseResources`** — nothing: the handler's destructor does it

Note:
    - Same three places as step 8. The threads are not ours to stop any more: the context keeps the pool while any handler exists.
    - A contract from a file carries no geometry; the plugin patches it from the host and prepares again when the host changes it.
    - The tensor is built once: planar float32 over the channel pointers JUCE hands out; the sample count is its shape.

---

<div class="tag">Take away</div>

## The main thread plans,<br><span class="then">the audio thread moves samples, the pool runs models.</span>

<div class="statement-points">
  <div>Two JSON files describe the model and the run</div>
  <div>Three calls connect them to the host</div>
  <div>Everything we built in part 1 is in the boxes between</div>
</div>

Note:
    - Hands-on from here: the part 1 model on anira, then the model taken apart.
