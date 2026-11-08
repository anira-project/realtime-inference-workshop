<h1>Outlook</h1>

v3: inline execution, GPU, NPU, zero copy

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- [before: straight on from step 10's done slide, no break]
- All four are planned for the final v3, due this month.
- Zero copy, the frame that stays where it is, comes last; first, the latency and inline execution.

---

<div class="tag">Latency</div>

## A big part is the inference.<br><span class="then">What if the engine is fast and real-time safe?</span>

<div class="lat-sum">
  <div class="lat-part">block adaptation<strong>1536</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part focus">inference<small>on the pool</small><strong>1024</strong></div>
  <div class="lat-plus">+</div>
  <div class="lat-part">the model itself<strong>2224</strong></div>
  <div class="lat-plus">=</div>
  <div class="lat-part total">latency<strong>4784</strong></div>
</div>

<div class="cards-note">host blocks of exactly 512 · declared 1 to 512, as in step 9: 5295</div>

<div class="lat-why">Run it on the audio thread, inside <code>process</code>: does anira support that?</div>

Note:
- Can we lower the latency further?
- This is step 7's sum, host blocks of exactly 512; step 9 declared one to 512, hence its higher number.
- The middle term, step 6's worker, is about a fifth: it is there because the pool runs the model.
- [pause] A fast, real-time safe engine could skip the pool and run the model in the callback itself.
- Step 4 tells us when that would fit at all.

---

<!-- .slide: class="recap" -->

<div class="tag">Part 1 · Step 4</div>

## Inline, the callback runs the model.<br><span class="then">Does it fit into one callback?</span>

<div class="deadline-pair">
  <div class="deadline fragment" data-fragment-index="1">
    <div class="deadline-budget"><span>host block 512 · 11.6 ms</span></div>
    <div class="deadline-bars"><i style="--i:0"></i><i style="--i:1"></i><i style="--i:2"></i><i class="runs" style="--i:3;height:33.5%"><span>forward pass<br>13 ms ✗</span></i></div>
    <div class="deadline-axis"><span>callbacks 1–3 · only copy</span><span>callback 4 · runs the model</span></div>
  </div>
  <div class="deadline fits fragment" data-fragment-index="2">
    <div class="deadline-budget" style="bottom: calc(40px + (100% - 40px) * 0.97)"><span>host block 2048 · 46.4 ms · worst case 18 ms ✓</span></div>
    <div class="deadline-bars"><i class="runs" style="--i:0;height:33.5%"><span>forward pass<br>13 ms ✓</span></i></div>
    <div class="deadline-axis"><span></span><span>every callback · runs the model</span></div>
  </div>
</div>

Note:
- Back to part 1, step 4, with Lina's numbers from this morning.
- [click] Host blocks of 512: too long for the fourth callback, and 13 ms is only the average.
- [click] With host blocks of 2048, one whole model block, even Lina's worst case of 18 ms fits easily.
- Yours too: if step 4's header holds your worst case, the static assert already proved it under 46.4 ms.
- So with a host block the size of the model block, the model could run inline.
- Could, because step 5 still stands: the engine has to keep the audio thread's rules.
- Two conditions, then, the rules and the time: the next slide checks both.

---

<div class="tag">v3 · Preview</div>

## Inline execution.<br><span class="then">No inference term in the latency.</span>

<div class="inline-tree">
  <div class="it-checks">
    <div class="it-check fragment" data-fragment-index="1"><span class="it-box">✓</span><div><b>real-time safe</b><small>the engine declares it: <code>REALTIME_SAFE</code> · none of the built-in engines does, your own can</small></div></div>
    <div class="it-check fragment" data-fragment-index="2"><span class="it-box">✓</span><div><b>fits into the callback</b><small>measured at <code>prepare</code>, within <code>inline_ratio</code> of the block period</small></div></div>
  </div>
  <div class="it-example fragment" data-fragment-index="3">Host blocks of 2048, the model's block size: nothing to adapt</div>
  <div class="it-branches fragment" data-fragment-index="3">
    <div class="it-branch yes">
      <div class="it-label">both ✓</div>
      <div class="it-thread">audio thread · <code>inline</code></div>
      <div class="lat-sum it-sum"><div class="lat-part">adaptation<small>2048 − 2048</small><strong>0</strong></div><div class="lat-plus">+</div><div class="lat-part focus">inference<strong>0</strong></div><div class="lat-plus">+</div><div class="lat-part">model<strong>2224</strong></div><div class="lat-plus">=</div><div class="lat-part total">latency<strong>2224</strong></div></div>
    </div>
    <div class="it-branch no">
      <div class="it-label">any ✗</div>
      <div class="it-thread">inference thread · the pool</div>
      <div class="lat-sum it-sum"><div class="lat-part">adaptation<small>2048 − 2048</small><strong>0</strong></div><div class="lat-plus">+</div><div class="lat-part">inference<strong>2048</strong></div><div class="lat-plus">+</div><div class="lat-part">model<strong>2224</strong></div><div class="lat-plus">=</div><div class="lat-part total">latency<strong>4272</strong></div></div>
    </div>
  </div>
</div>

<div class="cards-note">Planned contract key: <code>execution: worker | inline | auto</code>, fixed at <code>prepare</code></div>

<div class="cards-note">Not yet: it comes with the final v3 release, together with GPU and NPU accelerators. Until then every inference runs on the pool.</div>

Note:
- [click] First condition: real-time safe is the engine's promise; anira cannot check it.
- None of the built-in engines sets it: step 5 showed why, for ONNX Runtime.
- Your own engine can: a small model behind Fares's four calls, written to step 5's rules.
- [click] Second, it fits: a planned inline ratio caps the model's share of the callback.
- [click] Both met, the audio thread runs it; if either fails, the pool: one host block of waiting.
- [pause] Inline is almost half the latency: 50 ms instead of 97.
- On a mix bus that hardly matters; playing live, it does.
- A planned contract key, execution, picks the mode: worker, inline or auto; you just saw auto.
- In that plan, a forced inline that does not fit is refused at prepare.

---

<div class="tag">Where the memory is</div>

## A tensor says where it lives.<br><span class="then">CPU today. GPU and NPU follow.</span>

<div class="cards">
  <div class="card accent">{{ICON:chip}}<div class="card-title">CPU</div><div class="card-text">today: host memory, every engine</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:chip}}<div class="card-title">GPU</div><div class="card-text">later: CUDA, Metal, Vulkan, GL</div></div>
  <div class="card-arrow">→</div>
  <div class="card">{{ICON:chip}}<div class="card-title">NPU</div><div class="card-text">later: LiteRT and ExecuTorch delegates</div></div>
</div>

<div class="cards-note">The provider is the engine's twin: one plan per engine and provider, picked at prepare.</div>

Note:
- Where the memory is: today every tensor lives in host memory, the memory of the CPU.
- Beside pointer, dtype, shape and strides, a tensor names where its memory is.
- With the final v3 that may be a buffer on the GPU.
- Why move it?
- Either the inference is faster there, or the data already is, as with a camera frame.
- The NPU card differs: a tensor cannot live on an NPU; anira reaches it through a provider.
- A provider is the engine's way onto the hardware, ONNX Runtime with CUDA or CoreML.
- The delegates on the card are the same thing: a hardware back end.
- One plan per engine and provider, as Fares said.
- alpha.3 already runs a device provider, from host memory: the engine's runtime moves the bytes.

---

<div class="tag">After v3.0</div>

## Which combination is fastest?<br><span class="then">After v3.0, we benchmark them systematically.</span>

<div class="combos" data-chains="0,2,0;0,3,2;1,1,1;4,5,3;2,6,0">
  <svg class="combos-svg" aria-hidden="true"><g class="combos-base"></g><g class="combos-hot fragment"></g></svg>
  <div class="combos-col"><div class="combos-head">Hardware</div><div class="combos-node">Apple Silicon</div><div class="combos-node">NVIDIA</div><div class="combos-node">AMD</div><div class="combos-node">Intel</div><div class="combos-node">Qualcomm</div></div>
  <div class="combos-col"><div class="combos-head">Execution provider</div><div class="combos-node">CPU</div><div class="combos-node">CUDA</div><div class="combos-node">CoreML</div><div class="combos-node">Metal</div><div class="combos-node">DirectML</div><div class="combos-node">Vulkan</div><div class="combos-node">WebGPU</div></div>
  <div class="combos-col"><div class="combos-head">Engine</div><div class="combos-node">ONNX Runtime</div><div class="combos-node">LibTorch</div><div class="combos-node">LiteRT</div><div class="combos-node">ExecuTorch</div><div class="combos-node">TFLite</div></div>
</div>

<div class="combos-row fragment">
  <div class="combos-audio"><span>Audio</span><b class="ok">CPU ✓</b><b class="open">GPU ?</b><em>barely studied</em></div>
</div>

<div class="combos-invite fragment">Measured some of these already? We would like to compare notes.</div>

Note:
- Every line here is a possible path, and a question, not an answer.
- [click] The marked chains are examples of what we benchmark after v3.0, for audio and video.
- [click] For audio, most work today runs on the CPU, and those paths meet real time.
- You measured one of them yourself today: ONNX Runtime on the CPU.
- On the GPU: a few examples, no systematic comparison that we know of.
- [ask the room, hands up] Who has run a model on the GPU inside a plugin?
- [click] If you have, find us during the conference.
- When that benchmark is published, we post it on our channels.
- One likely reason GPUs are rare in audio: the data's trip there and back; the next slide counts it.

---

<div class="tag">Zero copy</div>

## The frame stays where it is.<br><span class="then">Only the landmarks cross over.</span>

<div class="zc">
  <div class="zc-row">
    <div class="zc-label">With a copy</div>
    <div class="lat-sum zc-sum">
      <div class="lat-part">one 4K frame<small>3840 × 2160 × 4 B</small><strong>33.2 MB</strong></div>
      <div class="lat-plus">×</div>
      <div class="lat-part">frames<small>a second</small><strong>30</strong></div>
      <div class="lat-plus">×</div>
      <div class="lat-part">GPU → CPU → GPU<small>there and back</small><strong>2</strong></div>
      <div class="lat-plus">=</div>
      <div class="lat-part total">moved<strong>≈ 2 GB/s</strong></div>
    </div>
  </div>
  <div class="zc-row fragment" data-fragment-index="1">
    <div class="zc-label">Zero copy</div>
    <div class="lat-sum zc-sum">
      <div class="lat-part">landmarks<small>2 hands × 21 × 3 × 4 B</small><strong>504 B</strong></div>
      <div class="lat-plus">×</div>
      <div class="lat-part">frames<small>a second</small><strong>30</strong></div>
      <div class="lat-plus">=</div>
      <div class="lat-part total">moved<strong>≈ 15 KB/s</strong></div>
    </div>
  </div>
</div>

<div class="cards-note fragment" data-fragment-index="1">The camera texture is the hand tracker's tensor on the GPU. Only its landmarks cross over to the CPU: about 130,000 times less.</div>

Note:
- Zero copy, the last feature: in step 10 every frame is copied to the CPU, then to the engine.
- Take a 4K camera that can hand over each frame as a GPU texture.
- Today its pixels are copied out of the capture buffer for the tracker; a GPU plan copies them up again.
- [pause] About 2 gigabytes every second, bytes, not bits, before the model has done anything.
- [click] Zero copy: that camera texture feeds the tracker's tensor on the GPU; a handle travels, no pixels.
- That is what a tensor's location is for.
- Only the landmarks cross over, 21 points per hand: your mapping turns them into latents on the CPU.
- About 15 kilobytes a second instead of 2 gigabytes.
- Both costs of the copy go away: the time and the memory bandwidth.
- So much for the outlook; one slide on today's code.

---

<div class="tag">Take away</div>

## Same API.<br><span class="then">Another memory.</span>

<div class="statement-points">
  <div>The configs and the three calls stay</div>
  <div>A plan gains a provider; the tensor gains a location</div>
  <div>The real-time rules do not change: the pool runs the model, inline only an engine that declares itself real-time safe</div>
</div>

<div class="cards-note">Start today on v3.0.0-alpha.3: the calls stay, details may still move until 3.0, the changelog says how.</div>

Note:
- The configs and the three calls are meant to stay as you wrote them: create, prepare, process.
- And the real-time rules from this morning do not change.
- So you do not have to wait for 3.0: start on alpha.3, today's version, right after ADC.
- Before then a pre-release may still rename a detail: the changelog says how, and you rebuild, as Fares said.
- [pause] And behind you lies the day: the model by hand, on anira, and played with your hands.

---

<div class="tag">Bonus</div>

## Opening the model.<br><span class="then">Encoder, eight numbers, decoder.</span>

<div class="note-box">A bonus step for after the workshop, in the repository: <code>steps/bonus/</code>, its slides linked from its README.</div>

Note:
- One more step for after the workshop: the bonus.
- We cut the model in the middle: an encoder, eight numbers, a decoder.
- Two handlers on anira, and your code sits between them.
- You have the repository: the exercise is in steps/bonus, and its README links the slides.

---

## Thank you.

<div class="qr-row">
  <div class="qr">
    {{QR:HTTPS://GITHUB.COM/ANIRA-PROJECT/REALTIME-INFERENCE-WORKSHOP}}
    <div class="qr-label">Workshop</div>
    <div class="qr-url">github.com/anira-project/realtime-inference-workshop</div>
  </div>
  <div class="qr">
    {{QR:HTTPS://GITHUB.COM/ANIRA-PROJECT/ANIRA}}
    <div class="qr-label">anira</div>
    <div class="qr-url">github.com/anira-project/anira</div>
  </div>
</div>

Note:
- Thank you very much for spending this day with us.
- Thanks to Lina and Fares, and to everyone who walked around and helped.
- Left, the workshop repository: every step, every solution and these slides; right, anira.
- [pause, let them scan]
- Your feedback helps us: open an issue there, or find us in the next three days.
- [ask the room, Lina and Fares come up, the QR codes stay up] Questions?
- Enjoy the conference.
