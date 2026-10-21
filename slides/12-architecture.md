<h1>The whole picture</h1>

Where the pieces sit

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - One diagram, built up in the order a call travels, column by column. Then the stages, then the plugin.

---

<div class="tag">Architecture</div>

## From your callback to the engine<br><span class="then">and back.</span>

<svg class="ov" viewBox="0 0 1600 720" aria-label="anira architecture">
  <defs>
    <marker id="ov-arrow" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="5" markerHeight="5" orient="auto-start-reverse">
      <path d="M0 0 L10 5 L0 10z" class="ov-arrowhead"/>
    </marker>
  </defs>
  <rect class="ov-thread audio" x="20" y="0" width="1210" height="38" rx="6"/>
  <text x="625" y="25" class="ov-thread-label">audio thread · main thread</text>
  <rect class="ov-thread pool" x="1300" y="0" width="280" height="38" rx="6"/>
  <text x="1440" y="25" class="ov-thread-label">inference threads</text>
  <text x="10" y="150" class="ov-row" transform="rotate(-90 10 150)">create</text>
  <text x="10" y="310" class="ov-row" transform="rotate(-90 10 310)">prepare</text>
  <text x="10" y="540" class="ov-row" transform="rotate(-90 10 540)">process</text>
  <g class="ov-col">
    <text x="160" y="70" class="ov-col-label">yours</text>
    <rect class="ov-box" x="20" y="84" width="280" height="616" rx="10"/>
    <text x="160" y="118" class="ov-title">your app</text>
    <rect class="ov-pill" x="36" y="134" width="118" height="30" rx="6"/><text x="95" y="154" class="ov-pill-text">model.json</text>
    <rect class="ov-pill" x="164" y="134" width="120" height="30" rx="6"/><text x="224" y="154" class="ov-pill-text">contract.json</text>
    <rect class="ov-pill dim" x="36" y="172" width="248" height="30" rx="6"/><text x="160" y="192" class="ov-pill-text">context.json · optional</text>
    <line class="ov-sep" x1="20" y1="236" x2="300" y2="236"/>
    <text x="160" y="268" class="ov-sub">prepareToPlay</text>
    <rect class="ov-pill" x="36" y="286" width="118" height="30" rx="6"/><text x="95" y="306" class="ov-pill-text">block size</text>
    <rect class="ov-pill" x="164" y="286" width="120" height="30" rx="6"/><text x="224" y="306" class="ov-pill-text">sample rate</text>
    <line class="ov-sep" x1="20" y1="376" x2="300" y2="376"/>
    <text x="160" y="408" class="ov-sub">processBlock</text>
    <rect class="ov-pill" x="66" y="446" width="160" height="30" rx="6"/><text x="146" y="466" class="ov-pill-text">input tensor</text>
    <rect class="ov-pill" x="66" y="606" width="160" height="30" rx="6"/><text x="146" y="626" class="ov-pill-text">output tensor</text>
    <line class="ov-flow" x1="284" y1="149" x2="346" y2="149" marker-end="url(#ov-arrow)"/>
    <line class="ov-flow" x1="284" y1="301" x2="346" y2="301" marker-end="url(#ov-arrow)"/>
    <line class="ov-flow strong" x1="226" y1="461" x2="346" y2="461" marker-end="url(#ov-arrow)"/>
    <line class="ov-flow strong" x1="346" y1="621" x2="226" y2="621" marker-end="url(#ov-arrow)"/>
  </g>
  <g class="ov-col fragment" data-fragment-index="1">
    <text x="1440" y="70" class="ov-col-label">private anira</text>
    <rect class="ov-box context" x="1300" y="84" width="280" height="616" rx="10"/>
    <text x="1440" y="118" class="ov-title">Context ‹static›</text>
    <rect class="ov-pill" x="1316" y="134" width="248" height="30" rx="6"/><text x="1440" y="154" class="ov-pill-text">one pool per process</text>
    <rect class="ov-pill" x="1316" y="172" width="248" height="30" rx="6"/><text x="1440" y="192" class="ov-pill-text">loaded models, pooled</text>
    <line class="ov-sep" x1="1300" y1="236" x2="1580" y2="236"/>
    <text x="1440" y="268" class="ov-sub">the queue</text>
    <rect class="ov-pill" x="1316" y="286" width="248" height="30" rx="6"/><text x="1440" y="306" class="ov-pill-text">lock-free, every handler</text>
    <line class="ov-sep" x1="1300" y1="376" x2="1580" y2="376"/>
    <g class="ov-threadrow"><rect x="1322" y="420" width="200" height="40" rx="8"/><text x="1422" y="446" class="ov-pill-text">thread 1 · inference</text></g>
    <g class="ov-threadrow"><rect x="1322" y="476" width="200" height="40" rx="8"/><text x="1422" y="502" class="ov-pill-text">thread 2 · inference</text></g>
    <g class="ov-threadrow"><rect x="1322" y="532" width="200" height="40" rx="8"/><text x="1422" y="558" class="ov-pill-text">thread 3 · inference</text></g>
    <circle class="ov-lens" cx="1532" cy="440" r="18"/>
    <line class="ov-lens-ray" x1="1532" y1="458" x2="1532" y2="604"/>
  </g>
  <g class="ov-col fragment" data-fragment-index="2">
    <text x="465" y="70" class="ov-col-label">public C ABI</text>
    <rect class="ov-box" x="330" y="84" width="270" height="616" rx="10"/>
    <text x="465" y="118" class="ov-title">anira_handler</text>
    <rect class="ov-pill" x="346" y="134" width="238" height="30" rx="6"/><text x="465" y="154" class="ov-pill-text">create(context, pipeline)</text>
    <line class="ov-sep" x1="330" y1="236" x2="600" y2="236"/>
    <rect class="ov-pill" x="346" y="286" width="238" height="30" rx="6"/><text x="465" y="306" class="ov-pill-text">prepare(contract)</text>
    <rect class="ov-pill" x="346" y="324" width="238" height="30" rx="6"/><text x="465" y="344" class="ov-pill-text">get_latency()</text>
    <line class="ov-sep" x1="330" y1="376" x2="600" y2="376"/>
    <rect class="ov-pill" x="346" y="446" width="238" height="30" rx="6"/><text x="465" y="466" class="ov-pill-text">process(in, out)</text>
    <text x="465" y="520" class="ov-note">nonblocking</text>
    <line class="ov-flow" x1="584" y1="149" x2="646" y2="149" marker-end="url(#ov-arrow)"/>
    <line class="ov-flow" x1="584" y1="301" x2="646" y2="301" marker-end="url(#ov-arrow)"/>
    <line class="ov-flow strong" x1="584" y1="461" x2="684" y2="461" marker-end="url(#ov-arrow)"/>
    <line class="ov-flow strong" x1="684" y1="621" x2="584" y2="621" marker-end="url(#ov-arrow)"/>
    <path class="ov-flow" d="M584 128 H1300" marker-end="url(#ov-arrow)"/>
    <text x="940" y="121" class="ov-note">get instance</text>
  </g>
  <g class="ov-col fragment" data-fragment-index="3">
    <text x="835" y="70" class="ov-col-label">private anira</text>
    <rect class="ov-box" x="630" y="84" width="410" height="616" rx="10"/>
    <text x="835" y="118" class="ov-title">session</text>
    <rect class="ov-pill" x="646" y="134" width="236" height="30" rx="6"/><text x="764" y="154" class="ov-pill-text">plans: engine × provider</text>
    <rect class="ov-pill" x="892" y="134" width="132" height="30" rx="6"/><text x="958" y="154" class="ov-pill-text">state pairs</text>
    <rect class="ov-pill" x="646" y="172" width="378" height="30" rx="6"/><text x="835" y="192" class="ov-pill-text">load the models, once per process</text>
    <line class="ov-sep" x1="630" y1="236" x2="1040" y2="236"/>
    <rect class="ov-pill" x="646" y="286" width="378" height="30" rx="6"/><text x="835" y="306" class="ov-pill-text">latency = adaptation + inference + model</text>
    <rect class="ov-pill" x="646" y="324" width="378" height="30" rx="6"/><text x="835" y="344" class="ov-pill-text">rings, slots, warm-up, budget</text>
    <line class="ov-sep" x1="630" y1="376" x2="1040" y2="376"/>
    <text x="700" y="420" class="ov-sub">ring</text>
    <circle class="ov-ring" cx="700" cy="461" r="16"/>
    <rect class="ov-pill stage" x="740" y="446" width="130" height="30" rx="6"/><text x="805" y="466" class="ov-pill-text">pre_process</text>
    <rect class="ov-pill" x="884" y="446" width="130" height="30" rx="6"/><text x="949" y="466" class="ov-pill-text">stamp time</text>
    <text x="700" y="580" class="ov-sub">ring</text>
    <circle class="ov-ring" cx="700" cy="621" r="16"/>
    <rect class="ov-pill stage" x="740" y="606" width="130" height="30" rx="6"/><text x="805" y="626" class="ov-pill-text">post_process</text>
    <rect class="ov-pill" x="884" y="606" width="130" height="30" rx="6"/><text x="949" y="626" class="ov-pill-text">next stamp</text>
    <line class="ov-flow strong" x1="716" y1="461" x2="740" y2="461" marker-end="url(#ov-arrow)"/>
    <line class="ov-flow strong" x1="870" y1="461" x2="884" y2="461"/>
    <line class="ov-flow strong" x1="884" y1="621" x2="870" y2="621" marker-end="url(#ov-arrow)"/>
    <line class="ov-flow strong" x1="740" y1="621" x2="716" y2="621" marker-end="url(#ov-arrow)"/>
    <path class="ov-flow" d="M1024 339 H1140 V190 H1300" marker-end="url(#ov-arrow)"/>
    <text x="1170" y="180" class="ov-note">start threads</text>
  </g>
  <g class="ov-col fragment" data-fragment-index="4">
    <text x="1165" y="400" class="ov-sub">slots</text>
    <rect class="ov-box" x="1100" y="412" width="130" height="172" rx="10"/>
    <rect class="ov-slot busy" x="1112" y="425" width="106" height="30" rx="5"/>
    <rect class="ov-slot busy" x="1112" y="481" width="106" height="30" rx="5"/>
    <rect class="ov-slot" x="1112" y="537" width="106" height="30" rx="5"/>
    <path class="ov-flow strong" d="M1014 461 H1060 V440 H1100" marker-end="url(#ov-arrow)"/>
    <path class="ov-flow strong" d="M1100 565 H1060 V621 H1014" marker-end="url(#ov-arrow)"/>
    <line class="ov-flow strong" x1="1218" y1="440" x2="1322" y2="440" marker-end="url(#ov-arrow)" marker-start="url(#ov-arrow)"/>
    <line class="ov-flow strong" x1="1218" y1="496" x2="1322" y2="496" marker-end="url(#ov-arrow)" marker-start="url(#ov-arrow)"/>
    <line class="ov-flow strong" x1="1218" y1="552" x2="1322" y2="552" marker-end="url(#ov-arrow)" marker-start="url(#ov-arrow)"/>
    <rect class="ov-lens-frame" x="1100" y="604" width="480" height="92" rx="8"/>
    <text x="1112" y="624" class="ov-note left">one inference, on a pool thread</text>
    <rect class="ov-pill stage" x="1112" y="636" width="140" height="28" rx="6"/><text x="1182" y="655" class="ov-pill-text small">before_inference</text>
    <line class="ov-flow strong" x1="1252" y1="650" x2="1276" y2="650" marker-end="url(#ov-arrow)"/>
    <rect class="ov-pill engine" x="1276" y="636" width="120" height="28" rx="6"/><text x="1336" y="655" class="ov-pill-text small">the engine</text>
    <text x="1336" y="684" class="ov-note">state_out → state_in</text>
    <line class="ov-flow strong" x1="1396" y1="650" x2="1420" y2="650" marker-end="url(#ov-arrow)"/>
    <rect class="ov-pill stage" x="1420" y="636" width="140" height="28" rx="6"/><text x="1490" y="655" class="ov-pill-text small">after_inference</text>
  </g>
</svg>

Note:
    - Your app, left: two files and three calls, from part 1's three places. The output tensor is the same memory as the input, in place.
    - Context, right: static, one per process. The pool, the queue, and the loaded models: ten instances of one model share one loaded engine object per plan.
    - The handler: the public surface. Every entry is a C function with a thread tag; create and prepare on the main thread, process nonblocking on the audio thread.
    - The session, private: at create the plans (one per engine and provider), the state pairs; at prepare the latency, the rings and slots, the warm-up that measures the budget.
    - In process: ring in, the stage's pre_process forms the model tensor, a stamp orders it, a slot carries it to a pool thread. Back the same way: next stamp, post_process, ring out.
    - Slots are preallocated at prepare, two per inference in flight. The audio thread claims a free one and leaves; a pool thread takes it from the queue and hands it back.
    - The lens: one inference on a pool thread is before_inference, the engine with the state pair swapped around it, after_inference. The next slide walks through it.
    - Idle pool threads spin with backoff or block on a semaphore: the context's wait strategy.
    - Nothing on the audio side allocates, locks or waits. That was steps 5 to 7.

---

<div class="tag">Stages</div>

## Four phases around the engine.<br><span class="then">Two threads, one state loop.</span>

<svg class="st" viewBox="0 0 1600 560" aria-label="the stage phases">
  <defs>
    <marker id="st-arrow" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="5" markerHeight="5" orient="auto-start-reverse">
      <path d="M0 0 L10 5 L0 10z" class="ov-arrowhead"/>
    </marker>
  </defs>
  <rect class="st-lane audio" x="20" y="40" width="1570" height="200" rx="10"/>
  <text x="40" y="70" class="st-lane-label">audio thread · driver-thread</text>
  <rect class="st-lane pool" x="20" y="300" width="1570" height="200" rx="10"/>
  <text x="40" y="330" class="st-lane-label">inference thread · one of the pool</text>
  <g class="fragment" data-fragment-index="1">
    <rect class="ov-pill" x="50" y="112" width="130" height="56" rx="8"/><text x="115" y="146" class="st-text">host block</text>
    <line class="ov-flow strong" x1="180" y1="140" x2="226" y2="140" marker-end="url(#st-arrow)"/>
    <circle class="ov-ring" cx="250" cy="140" r="24"/><text x="250" y="194" class="st-caption">input ring</text>
    <line class="ov-flow strong" x1="274" y1="140" x2="300" y2="140" marker-end="url(#st-arrow)"/>
    <rect class="ov-pill stage" x="300" y="112" width="170" height="56" rx="8"/><text x="385" y="146" class="st-text">pre_process</text>
    <text x="385" y="194" class="st-caption">a window out of the ring</text>
    <line class="ov-flow strong" x1="470" y1="140" x2="510" y2="140" marker-end="url(#st-arrow)"/>
    <rect class="ov-pill tensor" x="510" y="112" width="180" height="56" rx="8"/><text x="600" y="146" class="st-text">model tensor in</text>
    <text x="600" y="194" class="st-caption">a slot, stamped</text>
  </g>
  <g class="fragment" data-fragment-index="2">
    <path class="ov-flow strong" d="M600 168 V380" marker-end="url(#st-arrow)"/>
    <text x="616" y="262" class="st-caption left">the queue</text>
    <rect class="ov-pill stage" x="505" y="380" width="190" height="56" rx="8"/><text x="600" y="414" class="st-text">before_inference</text>
    <line class="ov-flow strong" x1="695" y1="408" x2="745" y2="408" marker-end="url(#st-arrow)"/>
    <rect class="ov-pill engine" x="745" y="372" width="150" height="72" rx="10"/><text x="820" y="414" class="st-text strong">engine</text>
    <line class="ov-flow strong" x1="895" y1="408" x2="945" y2="408" marker-end="url(#st-arrow)"/>
    <rect class="ov-pill stage" x="945" y="380" width="190" height="56" rx="8"/><text x="1040" y="414" class="st-text">after_inference</text>
    <path class="ov-flow strong" d="M1040 380 V168" marker-end="url(#st-arrow)"/>
    <text x="1056" y="262" class="st-caption left">the slot, back</text>
    <rect class="ov-pill tensor" x="945" y="112" width="190" height="56" rx="8"/><text x="1040" y="146" class="st-text">model tensor out</text>
    <text x="1040" y="194" class="st-caption">the next stamp</text>
    <line class="ov-flow strong" x1="1135" y1="140" x2="1180" y2="140" marker-end="url(#st-arrow)"/>
    <rect class="ov-pill stage" x="1180" y="112" width="170" height="56" rx="8"/><text x="1265" y="146" class="st-text">post_process</text>
    <text x="1265" y="194" class="st-caption">the result into the ring</text>
    <line class="ov-flow strong" x1="1350" y1="140" x2="1376" y2="140" marker-end="url(#st-arrow)"/>
    <circle class="ov-ring" cx="1400" cy="140" r="24"/><text x="1400" y="194" class="st-caption">output ring</text>
    <line class="ov-flow strong" x1="1424" y1="140" x2="1450" y2="140" marker-end="url(#st-arrow)"/>
    <rect class="ov-pill" x="1450" y="112" width="130" height="56" rx="8"/><text x="1515" y="146" class="st-text">host block</text>
  </g>
  <g class="fragment" data-fragment-index="3">
    <path class="ov-flow state" d="M880 444 V482 H760 V444" marker-end="url(#st-arrow)"/>
    <text x="820" y="470" class="st-caption">state_out → state_in</text>
    <text x="820" y="530" class="st-caption">the declared pair swaps around the engine, no copy</text>
  </g>
</svg>

<div class="cards-note">The default stage is step 2: a window out of the ring, the result back in. <code>before</code> and <code>after</code> are empty. You write a stage for another shape.</div>

Note:
    - Top lane, the audio thread: the host block goes into the input ring; pre_process takes one window out of it into the model tensor; the tensor sits in a slot, stamped, and goes into the queue. The callback returns.
    - Bottom lane, a pool thread: before_inference, the engine, after_inference, on the model tensors in place. Then the slot goes back.
    - Top lane again, a later callback: the oldest finished stamp is taken, post_process pushes the result into the output ring, and the host block is served from it.
    - The state loop sits around the engine itself, not around the stage: anira binds the declared pair to two buffers and swaps them per inference. What came out is what goes in next. No copy, and no code of yours.
    - Four phases, at most one stage per pipeline. pre and post are nonblocking: a stage that allocates there is your bug, and RTSan sees it.

---

<div class="tag">In a plugin</div>

## Three places, three calls.

**`prepareToPlay`** — geometry into the contract

```cpp
contract.hard_geometry(block, block, sample_rate);
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
