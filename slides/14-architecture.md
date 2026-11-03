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
- The session is private to each handler: plans and state pairs from create, latency, rings and slots from prepare.
- On the right the context, one per process: the pool, the queue and the loaded models, shared by every handler.
- A block crosses it in process and comes back the same way, in place, in the same buffer.
- Nothing on the audio side allocates or locks: the rules from steps 4 to 6, kept by the library.

---

<div class="tag">anira · C ABI</div>

## One C ABI.<br><span class="then">Every language on top of it.</span>

<div class="statement-points">
  <div><code>anira/abi/*.h</code>, C11: the only binary contract, the same on every platform and in WebAssembly</div>
  <div><code>anira.hpp</code>, header-only C++20, and the TypeScript package: one layer, two languages</div>
  <div>Versioned: append-only inside a major, stable from v3.0; alpha.2 is ABI 0.2</div>
  <div><code>compat/v2.hpp</code>: v2 code on the v3 library, unchanged</div>
</div>

Note:
- One C ABI, and every language sits on top of it.
- Why C: it is the one binary interface every compiler, platform and WebAssembly agree on.
- Only the anira_ functions are exported, so two plugins with anira inside do not clash in one host.
- A plugin that links anira statically has its own copy and its own pool; the instances of one plugin share it.
- anira.hpp, header-only C++20, and the TypeScript package sit on top of it.
- Main-thread calls return a status and an error message naming the field; audio-thread calls return a status, and rt_error says why.
- The ABI is append-only inside a major version and stable from v3.0; until then, rebuild against each pre-release.
- compat/v2.hpp runs v2 code unchanged, and the hand tracker in step 11 is exactly that.

---

<div class="tag">In a plugin</div>

## Three places, three calls.

<!-- .slide: class="tight-code" -->

**`prepareToPlay`** — the host blocks into the contract

```cpp
contract.hard_host_blocks(block, block, sample_rate);
anira_handler_prepare(handler, contract.native(), &error);
setLatencySamples(static_cast<int>(anira_handler_get_latency(handler, 0)));
```

**`processBlock`** — one tensor over the channel pointers, in place

```cpp
const int64_t shape[2] = {buffer.getNumChannels(), buffer.getNumSamples()};
anira_tensor_init_host_planar(&block, buffer.getArrayOfWritePointers(),
                              buffer.getNumChannels(), ANIRA_DTYPE_F32, 2, shape);
anira_handler_process(handler, &block, 0, &block, 0, nullptr);
```

**`releaseResources`** — nothing: our wrapper's destructor calls `anira_handler_destroy`

Note:
- Three places, three calls: the same three places as step 8.
- In prepareToPlay the host's blocks go into the contract, we prepare the handler, and we report its latency to the host.
- Prepare runs again whenever the host changes its blocks.
- The configs use the C++ handles of anira.hpp, and the handler calls are the C functions themselves, as in steps 9 and 10, so you see exactly what crosses the ABI.
- anira.hpp's InferenceHandler wraps the same three calls, if you prefer C++ all the way.
- In processBlock we put one tensor over the channel pointers JUCE hands out: planar float32, with the sample count as its shape.
- anira_handler_process then runs it in place.
- releaseResources does nothing: our small wrapper destroys the handler when it goes.
- The threads are not ours to stop any more: the context keeps the pool while any handler exists.

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
