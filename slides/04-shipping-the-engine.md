<h1>Step 4<br>Shipping the engine</h1>

LibTorch, ONNX Runtime, or writing it yourself

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - No exercise here. Talk, numbers, and a decision the room has to make for their own product.

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

---

<div class="tag">Motivation</div>

## The trade-off<br><span class="then">Less work vs. less weight.</span>

<div class="tradeoff">
  <div class="tradeoff-axis"><span>high level</span><span>low level</span></div>
  <div class="tradeoff-head"></div>
  <div class="tradeoff-head">weight</div>
  <div class="tradeoff-head">your work</div>
  <div class="tradeoff-name">LibTorch<small>export, load, call</small></div>
  <div class="tradeoff-bar weight" style="--v:100%;--i:0"><span>318 MB</span></div>
  <div class="tradeoff-bar work" style="--v:12%;--i:0"></div>
  <div class="tradeoff-name">ONNX Runtime<small>still easy</small></div>
  <div class="tradeoff-bar weight" style="--v:55%;--i:1"><span>24 MB</span></div>
  <div class="tradeoff-bar work" style="--v:28%;--i:1"></div>
  <div class="tradeoff-name">LiteRT<small>leaner, fewer ops</small></div>
  <div class="tradeoff-bar weight" style="--v:30%;--i:2"></div>
  <div class="tradeoff-bar work" style="--v:55%;--i:2"></div>
  <div class="tradeoff-name">Your own code<small>fastest, if you are good</small></div>
  <div class="tradeoff-bar weight" style="--v:3%;--i:3"><span>~ 0</span></div>
  <div class="tradeoff-bar work" style="--v:100%;--i:3"></div>
</div>

Note:
    - Weight on a log scale: unpacked runtime, macOS arm64, anira-project/backends v2.4.0. LiteRT's runtime was not measured unpacked; its download is 3 MB.
    - More high-level: less work. Export, load, call. The graph decides.
    - More low-level: smaller, more control. You carry the op coverage problem.
    - Hand-written: smallest and potentially fastest, and the only option where you own every bug.

---

<div class="tag">Engines</div>

## Libtorch<br><span class="then">PyTorch, minus Python.</span>

It is not an inference engine. It is **PyTorch**, minus Python:

- Autograd, the whole operator set, JIT, quantisation, sparse tensors
- Every kernel for every dtype, whether your model uses it or not
- `libtorch_cpu.dylib` is one 206 MB object; the linker cannot drop what it cannot see

Convenient, because the thing you traced is exactly the thing that runs.

---

<div class="tag">Engines</div>

## Hand-written <br><span class="then">Potentially the smallest and fastest.</span>

A GRU with 32 units, a small TCN, a biquad-shaped network: a few hundred lines of
plain C++, no dependency, no allocation, no graph.

**But:** vectorisation, cache layout, denormals, and every architecture change
means writing it again. Fast code is not the default — <b>it is a project</b>.

Note:
    - RTNeural as the known example. Ask who in the room has done it, and what it cost them.

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

---

<div class="tag">First step</div>

## Start with LibTorch<br><span class="then">Optimise against it.</span>

**Good first step**: LibTorch is a good and fast reference for correctness

We export both:

```
forward_stateful.pt     LibTorch, state inside the model
forward.onnx            ONNX, state in and out
```

Different `Engine` class, but same three methods `load()`, `forward()`, and `reset()`.

Note:
    - Libtorch is a good reference for correctness. Correctness first, then size. The LibTorch output is the reference the ONNX path must match.

---

<div class="tag">Deepedive</div>

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

---

<div class="tag">Take away</div>

## The choice of engine is important<br><span class="then">Affects performance anddistribution.</span>

<div class="statement-points">
  <div>Measure the size early — it is hard to walk back after a release</div>
  <div>Correct first, small second, fast third</div>
  <div>Keep the engine behind a thin interface, and the choice stays open</div>
</div>


Note:
    - Next: real-time safety, which no amount of size tuning fixes.
