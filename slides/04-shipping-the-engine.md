<h1>Step 4<br>Shipping the engine</h1>

LibTorch, ONNX Runtime, or writing it yourself

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - No exercise here. Talk, numbers, and a decision the room has to make for their own product.

**Script:**

"When we ship a plugin, we ship the engine too. Even if the model is small, but the engine can be huge. So we have to ask ourselves: Which engine do we pick? How much work do we want to do ourselves? How much control do we want? How much size can we afford? In this step we will not write any code. It's just a decision you'll have to make for your own product."

---

<div class="tag">Motivation</div>

## The problem<br><span class="then">The model is easy to ship. The engine is not.</span>

<span class="then">The engine is **~1,200×** the size of the program.</span>

Our step 1 binary:

```
step01_solution          262 KB
```

What has to sit next to it:

```
libtorch runtime         318 MB
  libtorch_cpu.dylib     206 MB   ← one file
```




Note:
    - Measured on this machine, macOS arm64, from the anira-project/backends v2.4.0 release.

**Script:**

"The model is just a file. The engine is a library. For example, our first step binary is only 262 KB, but the LibTorch engine is 318 MB and one of those dylibs alone is 206 MB. So the engine is about 1,200 times the size of the program. That is a lot of code that has to sit next to your plugin. And it has to be shipped on every install and every update.

---

<div class="tag">Motivation</div>

## Same model, different engine sizes</span>

macOS arm64, prebuilt, from `anira-project/backends` v2.4.0:

| engine | download | unpacked runtime |
|---|---|---|
| LibTorch 2.12 | 80 MB | **318 MB** |
| ONNX Runtime 1.26 | 6 MB | **24 MB** |
| ExecuTorch 1.3 (static) | 5 MB | — |
| LiteRT 2.1 | 3 MB | — |
| TFLite 2.17 | 2 MB | — |

Note:
    - Same weights, same output to 1e-6. 13× smaller for the ONNX path.

**Script:**

"Every model can be run with different engines. They will always have the same weights and same output to 1e-6, but they will have different performance characteristics and resource requirements. For example, for this model using LibTorch, unpacks to 318 MB, ONNX Runtime unpacks to 24 MB, which is 13 times smaller. LiteRT and ExecuTorch are only a few MB."

---

<div class="tag">Motivation</div>

## The trade-off<br><span class="then">Less work vs. less weight.</span>

<div class="tradeoff butterfly">
  <div class="tradeoff-axis"><span>high level</span><span>low level</span></div>
  <div class="tradeoff-head"></div>
  <div class="tradeoff-head weight">weight<small>log scale</small></div>
  <div class="tradeoff-head">your work</div>
  <div class="tradeoff-name">LibTorch</div>
  <div class="tradeoff-bar weight" style="--v:100%;--i:0"><span>318 MB</span></div>
  <div class="tradeoff-bar work" style="--v:10%;--i:0"><span>export, load, call</span></div>
  <div class="tradeoff-name">ONNX Runtime</div>
  <div class="tradeoff-bar weight" style="--v:55%;--i:1"><span>24 MB</span></div>
  <div class="tradeoff-bar work" style="--v:25%;--i:1"><span>check the ops</span></div>
  <div class="tradeoff-name">LiteRT</div>
  <div class="tradeoff-bar weight" style="--v:30%;--i:2"><span>3 MB download</span></div>
  <div class="tradeoff-bar work" style="--v:45%;--i:2"><span>convert, fewer ops</span></div>
  <div class="tradeoff-name">Your own code</div>
  <div class="tradeoff-bar weight" style="--v:3%;--i:3"><span>~ 0</span></div>
  <div class="tradeoff-bar work" style="--v:70%;--i:3"><span>write every layer</span></div>
</div>

Note:
    - Weight on a log scale: unpacked runtime, macOS arm64, anira-project/backends v2.4.0. LiteRT's runtime was not measured unpacked; its download is 3 MB.
    - More high-level: less work. Export, load, call. The graph decides.
    - More low-level: smaller, more control. You carry the op coverage problem.
    - Hand-written: smallest and potentially fastest, and the only option where you own every bug.

**Script:**

"But there is a trade-off for the size. The high-level engines will cost you less time to implement and maintain. You'll only have to export, load and call the model. The lower you go, the smaller it gets, but the more you'll have to do yourself. However, this also gives you more control. This means you have to balance the size of your engine with how much work you're willing to do and how much control you want."

---

<div class="tag">Engines</div>

## LibTorch<br><span class="then">PyTorch, minus Python.</span>

<div class="tradeoff butterfly mini">
  <div class="tradeoff-bar weight" style="--v:100%;--i:0"><span>318 MB</span></div>
  <div class="tradeoff-bar work" style="--v:10%;--i:0"><span>export, load, call</span></div>
</div>

<div class="cards getpay">
  <div class="card">
    <div class="card-title">You get</div>
    <ul class="checks">
      <li>The traced model is exactly what runs</li>
      <li>Every operator, every dtype</li>
      <li>Export, load, call</li>
    </ul>
  </div>
  <div class="card">
    <div class="card-title">You pay</div>
    <ul class="checks costs">
      <li>318 MB, one file alone is 206 MB</li>
      <li>Autograd, JIT, training: shipped, never used</li>
      <li>The linker cannot drop what it cannot see</li>
    </ul>
  </div>
</div>

Note:
    - It is not an inference engine. It is PyTorch, minus Python: autograd, the whole operator set, JIT, quantisation, sparse tensors, every kernel for every dtype.
    - libtorch_cpu.dylib is one 206 MB object, so the linker has nothing to strip.
    - The bar on top is the LibTorch row from the trade-off slide: the top end.

**Script:**

"LibTorch isn't really an inference engine. It's PyTorch without Python, a complete deep learning framework in C++. The benefits are that the traced model is exactly what runs. You can use every operator, every dtype and you only have to export, load and call the model. However, it is a very large framework. It has autograd, JIT, quantisation, sparse tensors, things that you probably don't need at this stage of development. libtorch_cpu.dylib is one finished 206 MB file, loaded when the program starts. Your linker never looks inside it, it just records “load this file”. So the whole file ships, used or not used. The linker can't strip what it can't see."

---

<div class="tag">Engines</div>

## Hand-written <br><span class="then">Potentially the smallest and fastest.</span>

<div class="tradeoff butterfly mini">
  <div class="tradeoff-bar weight" style="--v:3%;--i:0"><span>~ 0</span></div>
  <div class="tradeoff-bar work" style="--v:70%;--i:0"><span>write every layer</span></div>
</div>

<div class="cards getpay">
  <div class="card">
    <div class="card-title">You get</div>
    <ul class="checks">
      <li>~ 0 MB, no dependency</li>
      <li>No allocation, no graph</li>
      <li>Complete control over the implementation</li>
    </ul>
  </div>
  <div class="card">
    <div class="card-title">You pay</div>
    <ul class="checks costs">
      <li>Vectorisation and cache layout, by hand</li>
      <li>Denormals</li>
      <li>New architecture: write it again</li>
    </ul>
  </div>
</div>

<div class="cards-note fragment">Fast code is not the default — <b>it is a project</b>.</div>

Note:
    - A GRU with 32 units, a small TCN, a biquad-shaped network: a few hundred lines of plain C++.
    - The bar on top is the "your own code" row from the trade-off slide: the bottom end.
    - RTNeural as the known example. Ask who in the room has done it, and what it cost them.

**Script:**

*Ask: "Who here has hand-written inference? What did it cost you?"*
"A hand-written inference engine is a custom implementation of the neural network in C++ or another low-level language. It's the smallest and potentially fastest option, but it requires a lot of work to implement and maintain. The benefits are that it is potentially 0 MB in size, you have complete control over the implementation, and you can optimise it for your specific use case.
For small networks, like a 32-unit GRU or a small TCN, a few hundred lines of C++ can do it. However you have to handle vectorisation, cache layout and denormals yourself. That's a project in itself."

---

<div class="tag">Shipping</div>

## Loading a plugin<br><span class="then">Users download the engine too.</span>

<span>A VST3 is a bundle. Your engine goes inside it, per architecture (universal binary):</span>

<div class="tradeoff ships">
  <div class="tradeoff-name">LibTorch</div>
  <div class="tradeoff-bar weight" style="--v:100%;--i:0"><span>&gt; 600 MB</span></div>
  <div class="tradeoff-name">ONNX Runtime</div>
  <div class="tradeoff-bar weight" style="--v:8.3%;--i:1"><span>~ 50 MB</span></div>
  <div class="tradeoff-name">hand-written</div>
  <div class="tradeoff-bar weight" style="--v:0.4%;--i:2"><span>~ 0</span></div>
</div>

<p class="ships-note">Every user downloads that, on every install and every update.</p>

Note: Users download that. Installers, notarisation, CDN bills, and the first
impression of your product.

**Script:**

"Remember your users download this when they download your plugin. In a universal-binary VST3, LibTorch is over
600 MB, on every install and every update. This might be a problem for your users. It's a lot of data to download and install. That's installers, notarisation, CDN bills, and the first impression of your product."

---

<div class="tag">First step</div>

## Start with LibTorch<br><span class="then">Optimise against it.</span>

**Good first step**: First LibTorch<br> **then** lower lever engine.

For this workshop:

```
model.pt                LibTorch, state inside the model
model_forward.onnx      ONNX, state in and out
```

Different `Engine` class, but same three methods `load()`, `forward()`, and `reset()`.

Note:
    - Libtorch is a good reference for correctness. Correctness first, then size. The LibTorch output is the reference the ONNX path must match.

**Script:**

"So, our advice is: start with LibTorch as the correctness reference, then optimise against it. For this workshop we've export both a `.pt` and an `.onnx`. They are different engine classes, but have the same three methods."

---

<div class="tag">Deep dive</div>

## Stateless vs. stateful<br><span class="then">Some models remember others read the past at every call.</span>

<div class="state-compare">
  <div class="state-panel">
    <div class="card-title">Stateless</div>
    <svg class="state-svg stateless" viewBox="0 0 860 380" aria-hidden="true">
  <g class="sv-window">
    <rect class="sv-fill past" x="20.0" y="30" width="273.3" height="90"/>
    <rect class="sv-fill now" x="293.3" y="30" width="136.7" height="90"/>
    <path class="sv-line" d="M20.0 134 v12 H430.0 v-12"/>
    <path class="sv-line" d="M225.0 146 V200"/><path class="sv-head" d="M218.0 192 l7 10 l7 -10z"/>
    <rect class="sv-model" x="145.0" y="206" width="160" height="60" rx="8"/>
    <text x="225.0" y="244" class="sv-label strong">model</text>
  </g>
  <rect class="sv-block" x="20.0" y="30" width="136.7" height="90"/><rect class="sv-block" x="156.7" y="30" width="136.7" height="90"/><rect class="sv-block" x="293.3" y="30" width="136.7" height="90"/><rect class="sv-block" x="430.0" y="30" width="136.7" height="90"/><rect class="sv-block" x="566.7" y="30" width="136.7" height="90"/><rect class="sv-block" x="703.3" y="30" width="136.7" height="90"/>
  <path class="sv-wave" d="M20 75.0 L24 80.8 L28 86.4 L32 90.9 L36 93.5 L40 93.6 L44 91.0 L48 85.9 L52 78.9 L56 70.9 L60 63.0 L64 56.3 L68 51.9 L72 50.5 L76 52.5 L80 57.8 L84 65.6 L88 75.0 L92 84.7 L96 93.3 L100 99.5 L104 102.5 L108 101.7 L112 97.3 L116 89.7 L120 80.1 L124 69.9 L128 60.3 L132 52.8 L136 48.4 L140 47.7 L144 50.7 L148 56.9 L152 65.4 L156 75.0 L160 84.2 L164 91.9 L168 97.0 L172 98.9 L176 97.5 L180 93.2 L184 86.7 L188 78.9 L192 71.2 L196 64.5 L200 59.6 L204 57.2 L208 57.3 L212 59.8 L216 64.1 L220 69.4 L224 75.0 L228 80.0 L232 83.8 L236 86.0 L240 86.5 L244 85.4 L248 83.0 L252 79.9 L256 76.6 L260 73.5 L264 71.1 L268 69.6 L272 69.0 L276 69.3 L280 70.3 L284 71.8 L288 73.4 L292 75.0 L296 76.3 L300 77.3 L304 77.9 L308 78.0 L312 77.8 L316 77.3 L320 76.5 L324 75.5 L328 74.5 L332 73.4 L336 72.4 L340 71.7 L344 71.3 L348 71.4 L352 72.1 L356 73.3 L360 75.0 L364 77.1 L368 79.2 L372 81.1 L376 82.3 L380 82.7 L384 81.9 L388 79.9 L392 76.8 L396 73.0 L400 68.9 L404 65.2 L408 62.5 L412 61.2 L416 61.9 L420 64.6 L424 69.2 L428 75.0 L432 81.4 L436 87.5 L440 92.3 L444 95.0 L448 95.0 L452 92.1 L456 86.7 L460 79.2 L464 70.7 L468 62.4 L472 55.4 L476 50.9 L480 49.5 L484 51.7 L488 57.2 L492 65.3 L496 75.0 L500 84.9 L504 93.6 L508 99.9 L512 102.8 L516 101.9 L520 97.3 L524 89.7 L528 80.1 L532 69.9 L536 60.5 L540 53.1 L544 48.9 L548 48.3 L552 51.3 L556 57.5 L560 65.8 L564 75.0 L568 83.9 L572 91.2 L576 96.0 L580 97.7 L584 96.3 L588 92.1 L592 85.9 L596 78.7 L600 71.5 L604 65.2 L608 60.8 L612 58.7 L616 58.9 L620 61.2 L624 65.2 L628 70.0 L632 75.0 L636 79.4 L640 82.8 L644 84.6 L648 85.0 L652 84.0 L656 81.9 L660 79.2 L664 76.4 L668 73.8 L672 71.7 L676 70.5 L680 70.0 L684 70.3 L688 71.1 L692 72.3 L696 73.7 L700 75.0 L704 76.2 L708 77.0 L712 77.6 L716 77.8 L720 77.7 L724 77.3 L728 76.5 L732 75.6 L736 74.4 L740 73.2 L744 72.1 L748 71.2 L752 70.7 L756 70.7 L760 71.5 L764 72.9 L768 75.0 L772 77.4 L776 80.0 L780 82.2 L784 83.6 L788 84.0 L792 83.0 L796 80.7 L800 77.1 L804 72.7 L808 68.1 L812 64.0 L816 60.9 L820 59.7 L824 60.5 L828 63.6 L832 68.6 L836 75.0 L840 82.0"/>
  <rect class="sv-legend past" x="210" y="316" width="18" height="18" rx="3"/><text x="236" y="331" class="sv-label left">past, read again</text>
  <rect class="sv-legend now" x="440" y="316" width="18" height="18" rx="3"/><text x="466" y="331" class="sv-label left">current block</text>
</svg>
  </div>
  <div class="state-panel">
    <div class="card-title">Stateful</div>
    <svg class="state-svg stateful" viewBox="0 0 860 380" aria-hidden="true">
  <g class="sv-call c0">
    <rect class="sv-fill now" x="293.3" y="30" width="136.7" height="90"/>
    <path class="sv-line" d="M361.7 128 V200"/><path class="sv-head" d="M354.7 192 l7 10 l7 -10z"/>
    <rect class="sv-model" x="319.7" y="206" width="84" height="60" rx="8"/>
    <text x="361.7" y="244" class="sv-label strong">call 1</text>
  </g><g class="sv-state s0">
    <path class="sv-line state" d="M407.7 236 H444.3"/><path class="sv-head state" d="M442.3 228 l12 8 l-12 8z"/>
  </g><g class="sv-call c1">
    <rect class="sv-fill now" x="430.0" y="30" width="136.7" height="90"/>
    <path class="sv-line" d="M498.3 128 V200"/><path class="sv-head" d="M491.3 192 l7 10 l7 -10z"/>
    <rect class="sv-model" x="456.3" y="206" width="84" height="60" rx="8"/>
    <text x="498.3" y="244" class="sv-label strong">call 2</text>
  </g><g class="sv-state s1">
    <path class="sv-line state" d="M544.3 236 H581.0"/><path class="sv-head state" d="M579.0 228 l12 8 l-12 8z"/>
  </g><g class="sv-call c2">
    <rect class="sv-fill now" x="566.7" y="30" width="136.7" height="90"/>
    <path class="sv-line" d="M635.0 128 V200"/><path class="sv-head" d="M628.0 192 l7 10 l7 -10z"/>
    <rect class="sv-model" x="593.0" y="206" width="84" height="60" rx="8"/>
    <text x="635.0" y="244" class="sv-label strong">call 3</text>
  </g><g class="sv-state s2">
    <path class="sv-line state" d="M681.0 236 H717.7"/><path class="sv-head state" d="M715.7 228 l12 8 l-12 8z"/>
  </g><g class="sv-call c3">
    <rect class="sv-fill now" x="703.3" y="30" width="136.7" height="90"/>
    <path class="sv-line" d="M771.7 128 V200"/><path class="sv-head" d="M764.7 192 l7 10 l7 -10z"/>
    <rect class="sv-model" x="729.7" y="206" width="84" height="60" rx="8"/>
    <text x="771.7" y="244" class="sv-label strong">call 4</text>
  </g>
  <rect class="sv-block" x="20.0" y="30" width="136.7" height="90"/><rect class="sv-block" x="156.7" y="30" width="136.7" height="90"/><rect class="sv-block" x="293.3" y="30" width="136.7" height="90"/><rect class="sv-block" x="430.0" y="30" width="136.7" height="90"/><rect class="sv-block" x="566.7" y="30" width="136.7" height="90"/><rect class="sv-block" x="703.3" y="30" width="136.7" height="90"/>
  <path class="sv-wave" d="M20 75.0 L24 80.8 L28 86.4 L32 90.9 L36 93.5 L40 93.6 L44 91.0 L48 85.9 L52 78.9 L56 70.9 L60 63.0 L64 56.3 L68 51.9 L72 50.5 L76 52.5 L80 57.8 L84 65.6 L88 75.0 L92 84.7 L96 93.3 L100 99.5 L104 102.5 L108 101.7 L112 97.3 L116 89.7 L120 80.1 L124 69.9 L128 60.3 L132 52.8 L136 48.4 L140 47.7 L144 50.7 L148 56.9 L152 65.4 L156 75.0 L160 84.2 L164 91.9 L168 97.0 L172 98.9 L176 97.5 L180 93.2 L184 86.7 L188 78.9 L192 71.2 L196 64.5 L200 59.6 L204 57.2 L208 57.3 L212 59.8 L216 64.1 L220 69.4 L224 75.0 L228 80.0 L232 83.8 L236 86.0 L240 86.5 L244 85.4 L248 83.0 L252 79.9 L256 76.6 L260 73.5 L264 71.1 L268 69.6 L272 69.0 L276 69.3 L280 70.3 L284 71.8 L288 73.4 L292 75.0 L296 76.3 L300 77.3 L304 77.9 L308 78.0 L312 77.8 L316 77.3 L320 76.5 L324 75.5 L328 74.5 L332 73.4 L336 72.4 L340 71.7 L344 71.3 L348 71.4 L352 72.1 L356 73.3 L360 75.0 L364 77.1 L368 79.2 L372 81.1 L376 82.3 L380 82.7 L384 81.9 L388 79.9 L392 76.8 L396 73.0 L400 68.9 L404 65.2 L408 62.5 L412 61.2 L416 61.9 L420 64.6 L424 69.2 L428 75.0 L432 81.4 L436 87.5 L440 92.3 L444 95.0 L448 95.0 L452 92.1 L456 86.7 L460 79.2 L464 70.7 L468 62.4 L472 55.4 L476 50.9 L480 49.5 L484 51.7 L488 57.2 L492 65.3 L496 75.0 L500 84.9 L504 93.6 L508 99.9 L512 102.8 L516 101.9 L520 97.3 L524 89.7 L528 80.1 L532 69.9 L536 60.5 L540 53.1 L544 48.9 L548 48.3 L552 51.3 L556 57.5 L560 65.8 L564 75.0 L568 83.9 L572 91.2 L576 96.0 L580 97.7 L584 96.3 L588 92.1 L592 85.9 L596 78.7 L600 71.5 L604 65.2 L608 60.8 L612 58.7 L616 58.9 L620 61.2 L624 65.2 L628 70.0 L632 75.0 L636 79.4 L640 82.8 L644 84.6 L648 85.0 L652 84.0 L656 81.9 L660 79.2 L664 76.4 L668 73.8 L672 71.7 L676 70.5 L680 70.0 L684 70.3 L688 71.1 L692 72.3 L696 73.7 L700 75.0 L704 76.2 L708 77.0 L712 77.6 L716 77.8 L720 77.7 L724 77.3 L728 76.5 L732 75.6 L736 74.4 L740 73.2 L744 72.1 L748 71.2 L752 70.7 L756 70.7 L760 71.5 L764 72.9 L768 75.0 L772 77.4 L776 80.0 L780 82.2 L784 83.6 L788 84.0 L792 83.0 L796 80.7 L800 77.1 L804 72.7 L808 68.1 L812 64.0 L816 60.9 L820 59.7 L824 60.5 L828 63.6 L832 68.6 L836 75.0 L840 82.0"/>
  <path class="sv-line state" d="M200 325 H236"/><text x="246" y="331" class="sv-label left">state: the past, carried along</text>
</svg>
  </div>
</div>

Note:
    - Stateless: output depends only on this call's input; context has to come with the input (overlapping windows); calls are independent.
    - Stateful: memory from one call to the next (RNN hidden states, cached convolution history); calls in order, one state per stream, reset when the stream restarts.

**Script:**

"One more concept before we move on, because it explains the difference between the two exports.

A stateless model has no memory. Every call starts from zero. But most audio models need some context: to process this block well, they need to know what came just before. So a stateless model gets the past handed in again, every time. You see it here: the model gets the current block, in green, plus a piece of the past, in purple. Then the window moves on, and part of what was ‘now’ becomes ‘past’ and gets read again. So you compute the same samples several times, but every call stands on its own.

A stateful model works the other way round. It only gets the new block, and it carries the past along itself, inside, from one call to the next. That’s the dashed line: call 1 passes its memory to call 2, call 2 to call 3, and so on. That’s less work per call, and it’s what our model does.

But that memory has consequences, and we’ve already seen some of them:

- The calls have to be in order. You can’t skip a block or process two in parallel.
- One state per stream. If you have two channels, you need two states.
- You have to reset it when a new stream starts. That’s the reset() from step 1, and why creating the engine inside the loop was wrong: it threw the memory away every block.

And this is where the two engines differ. In the LibTorch file, the state is hidden inside the model, so we never see it. In the ONNX export, the state is an explicit input and output: we hand it in with every block and get the updated state back. It’s the same idea, you just carry it yourself.”

---

<div class="tag">Take away</div>

## The choice of engine is important<br><span class="then">Affects performance and distribution.</span>

1. Measure the size early
2. Correct first, small second, fast third
3. If you keep the engine behind a thin interface, the choice stays open


Note:
    - Next: real-time safety, which no amount of size tuning fixes.

**Script:**

"So the take-away is: measure size early, because it's hard to walk back after a release. Make sure your model is exported correctly first, then make it small, and if you can make it fast, do so. And when implementing, keep the engine behind a thin interface so the choice stays open and you can switch between them if needed. But, we're not there yet. We have to make sure we're real-time safe."
