<h1>The whole picture</h1>

One picture, one C ABI, three places

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- Now lets put it all together
- Everything from create, prepare and process in one picture, the C ABI underneath it, and the three places in a plugin where we call it

---

<div class="tag">Architecture</div>

## From your callback to the engine<br><span class="then">and back.</span>

{{MAP:show=create,prepare,process|abi}}

Note:
- This is the whole path from our callback to the engine and back
- On the left our app: the three files and the three calls
- The dashed line is the C ABI: everything right of it is the library, and the handler is all we touch
- The session belongs to one handler: the state pairs from create, and the plans, the latency, the rings and the slots from prepare
- On the right what anira keeps once per process: the pool, the queue and the loaded models, shared by every handler
- In process a block crosses the line and comes back the same way, in place, in the same buffer
- And nothing on the audio side allocates or locks: the rules from steps 5 to 7, now kept by the library

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
- Underneath all of this sits one C ABI, and every language sits on top of it
- Why C: it is the one binary interface every compiler, every platform and WebAssembly agree on
- Only the anira_ functions are exported, so two plugins with anira inside do not clash in one host: the hidden symbols from Create
- A plugin that links anira statically has its own copy and its own pool, and the instances of that plugin share it
- On top of it: anira.hpp, header-only C++20, and the TypeScript package
- Main-thread calls return a status and an error message that names the field; audio-thread calls return a status, and rt_error says why
- The ABI is append-only inside a major version and stable from v3.0. Until then, rebuild against each pre-release
- And compat/v2.hpp runs v2 code with one include swapped and using namespace anira::v2. It is deprecated and goes one minor release after 3.0. The hand tracker in step 10 uses exactly that

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
- And this is how it looks in a plugin: three places, three calls. The constructor, prepareToPlay and processBlock, as in step 8
- In the constructor we create: the model config, a pipeline with it, and the handler. It checks the config and loads nothing yet
- In prepareToPlay the host's blocks go into the contract, 1 up to the announced block size, we prepare the handler and tell the host its latency. And we prepare again whenever the host changes its blocks
- In processBlock one tensor goes over the host's channels as they are, planar: one pointer per channel, and the shape. Then process runs it in place
- Our model in step 9 is mono, so there we mix down first, as in step 8. A stereo model takes the buffer as it is
- Nothing to do in releaseResources: the handler destroys itself when the plugin goes, and the pool runs until the last handler is gone
- This is the pattern of anira's own JUCE example
- Hands-on from here: the part 1 model on anira, then the model taken apart
