<h1>The whole picture</h1>

One map, one C ABI, three places

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- The whole picture.
- One map with every row filled, the C ABI it sits on, and the three places in a plugin where all of it is called.

---

<div class="tag">Architecture</div>

## From your callback to the engine<br><span class="then">and back.</span>

{{MAP:show=create,prepare,process|abi}}

Note:
- This is the path from your callback to the engine and back, in one picture.
- On the left your app: the three files and the three calls.
- The dashed line is the C ABI: right of it is the library, and the handler is all you touch.
- The session is private to each handler: the state pairs from create; the plans, the latency, the rings and the slots from prepare.
- On the right what anira keeps once per process: the pool, the queue and the loaded models, shared by every handler.
- A block crosses it in process and comes back the same way, in place, in the same buffer.
- Nothing on the audio side allocates or locks: the rules from steps 5 to 7, kept by the library.

---

<div class="tag">anira · C ABI</div>

## One C ABI.<br><span class="then">Every language on top of it.</span>

<div class="statement-points">
  <div><code>anira/abi/*.h</code>, C11: the only binary contract, the same on every platform and in WebAssembly</div>
  <div><code>anira.hpp</code>, header-only C++20, and the TypeScript package: one layer, two languages</div>
  <div>Versioned: append-only inside a major, stable from v3.0; alpha.3 is ABI 0.3</div>
  <div><code>compat/v2.hpp</code>: v2 code on the v3 library: one include, one namespace; deprecated</div>
</div>

Note:
- One C ABI, and every language sits on top of it.
- Why C: it is the one binary interface every compiler, platform and WebAssembly agree on.
- Only the anira_ functions are exported, so two plugins with anira inside do not clash in one host.
- A plugin that links anira statically has its own copy and its own pool; the instances of one plugin share it.
- anira.hpp, header-only C++20, and the TypeScript package sit on top of it.
- Main-thread calls return a status and an error message naming the field; audio-thread calls return a status, and rt_error says why.
- The ABI is append-only inside a major version and stable from v3.0; until then, rebuild against each pre-release.
- compat/v2.hpp runs v2 code with one include swapped and using namespace anira::v2. It is deprecated and goes one minor release after 3.0. The hand tracker in step 11 is exactly that.

---

<div class="tag">In a plugin</div>

## Three places in a plugin.<br><span class="then">Three calls.</span>

<div class="places">
  <div class="place"><div class="place-head"><b>Constructor</b> · create</div>
<pre><code class="language-cpp">anira::Pipeline pipeline;
pipeline.add_inference(anira::ModelConfig::from_file("model.json"));
m_handler.emplace(m_context, pipeline);</code></pre></div>
  <div class="place"><div class="place-head"><b>prepareToPlay</b> · prepare</div>
<pre><code class="language-cpp">m_contract.hard_host_blocks(1, block, sample_rate);
m_handler-&gt;prepare(m_contract);
setLatencySamples(int(m_handler-&gt;get_latency(0)));</code></pre></div>
  <div class="place"><div class="place-head"><b>processBlock</b> · process</div>
<pre><code class="language-cpp">// one planar tensor over the host's channels
auto block = anira::Tensor::from_host_planar(channels, shape);
m_handler-&gt;process(block, 0, block, 0);</code></pre></div>
</div>

Note:
- Three places in a plugin, and three calls: the constructor, prepareToPlay and processBlock, as in step 8.
- In the constructor we create: the model config, a pipeline with it, and the handler. Create checks the model config and builds the slots; it loads nothing yet.
- In prepareToPlay the host's blocks go into the contract, 1 up to the block size it announces, we prepare the handler, and we report its latency to the host. Prepare runs again whenever the host changes its blocks.
- In processBlock one tensor goes over the host's channels as they are, planar: channels is a span with one pointer per channel, filled from the buffer every block, and shape is the channel count and the sample count. float32 comes from the pointers. process then runs it in place.
- Our model in steps 9 and 10 is mono, one channel in the model file, so there we mix down first, as in step 8. A stereo model takes the buffer as it is.
- Nothing to do in releaseResources: the handler destroys itself when the plugin goes.
- The threads are not ours to stop any more: anira keeps the pool until the last handler is destroyed.
- This is the pattern of anira's own JUCE example.
---

<div class="tag">Take away</div>

## The main thread plans,<br><span class="then">the audio thread moves samples, the pool runs models.</span>

<div class="statement-points">
  <div>A model file and a contract file, plus an optional context file</div>
  <div>Three calls connect them to the host</div>
  <div>Everything we built in part 1 is in the boxes between</div>
</div>

Note:
- The main thread plans, the audio thread moves samples, the pool runs models.
- A model file and a contract file describe the model and the run, and a third, optional one the context.
- Three calls connect them to the host.
- Everything we built in part 1 is in the boxes between.
- Hands-on from here: the part 1 model on anira, then the model taken apart.
