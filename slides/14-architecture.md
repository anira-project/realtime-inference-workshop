<h1>The whole picture</h1>

One map, one C ABI, three places

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - The map with every row filled, the C ABI it sits on, and the three places in a plugin where all of it is called.

---

<div class="tag">Architecture</div>

## From your callback to the engine<br><span class="then">and back.</span>

{{MAP:show=create,prepare,process|abi}}

Note:
    - Your app, left: the files and three calls, from part 1's three places. The output tensor is the same memory as the input, in place.
    - The dashed line is the C ABI: everything right of it is the library, and the handler is its public surface. Every entry is a C function with a thread tag.
    - The session, private: at create the plans (one per engine and provider) and the state pairs; at prepare the latency, the rings and slots, the warm-up.
    - The context, right: one per process. The pool, the queue, and the loaded models, shared by every handler that runs them.
    - In process: ring in, the stage's pre_process forms the model tensor, a stamp orders it, a slot carries it to a pool thread. Back the same way: next stamp, post_process, ring out.
    - Nothing on the audio side allocates, locks or waits. That was steps 4 to 6.

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
    - Every entry point is an exported anira_* function. The library's own C++, tanh-lib and the queue are private: a plugin that links anira statically exports nothing of it.
    - Errors: every call returns a status, and a failed main-thread call fills an anira_error with a message naming the field. The audio-thread entries carry no error struct; rt_error says why one failed.
    - Through the pre-releases the major is 0 and the check demands an exact match: rebuild against each alpha. From v3.0 the promise holds inside major 3.
    - The hand tracker in step 11 is v2 code; it runs through compat/v2.hpp.

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

**`releaseResources`** — nothing: the handler's destructor does it

Note:
    - Same three places as step 8. The threads are not ours to stop any more: the context keeps the pool while any handler exists.
    - A contract from a file carries no host blocks; the plugin patches it from the host and prepares again when the host changes it.
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
