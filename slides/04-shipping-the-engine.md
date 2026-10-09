<h1>Step 4<br>Shipping the engine</h1>

LibTorch, ONNX Runtime, or writing it yourself

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - No exercise here. Talk, numbers, and a decision the room has to make for their own product.

---

<div class="tag">Shipping</div>

## 262 KB of program.<br><span class="then">318 MB of engine.</span>

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

<div class="tag">Engines</div>

## Same model, 13× smaller.

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

<div class="tag">LibTorch</div>

## It is PyTorch, minus Python.

It is not an inference engine. It is **PyTorch**, minus Python:

- Autograd, the whole operator set, JIT, quantisation, sparse tensors
- Every kernel for every dtype, whether your model uses it or not
- `libtorch_cpu.dylib` is one 206 MB object; the linker cannot drop what it cannot see

Convenient, because the thing you traced is exactly the thing that runs.

---

<div class="tag">The trade-off</div>

## Less work, or less weight.

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

<div class="tag">Hand-written</div>

## Potentially the smallest and fastest.<br><span class="then">At the cost of maintaining it yourself.</span>

A GRU with 32 units, a small TCN, a biquad-shaped network: a few hundred lines of
plain C++, no dependency, no allocation, no graph.

**But:** vectorisation, cache layout, denormals, and every architecture change
means writing it again. Fast code is not the default — it is a project.

Note:
    - RTNeural as the known example. Ask who in the room has done it, and what it cost them.

---

<div class="tag">What ships</div>

## Users download the engine too.

A VST3 is a bundle. Your engine goes inside it, per architecture (universal binary):

<div class="tradeoff ships">
  <div class="tradeoff-name">LibTorch</div>
  <div class="tradeoff-bar weight" style="--v:100%;--i:0"><span>&gt; 600 MB</span></div>
  <div class="tradeoff-name">ONNX Runtime</div>
  <div class="tradeoff-bar weight" style="--v:8.3%;--i:1"><span>~ 50 MB</span></div>
  <div class="tradeoff-name">hand-written</div>
  <div class="tradeoff-bar weight" style="--v:0.4%;--i:2"><span>~ 0</span></div>
</div>

Users download that. Installers, notarisation, CDN bills, and the first
impression of your product.

---

<div class="tag">LibTorch first</div>

## Start with a correct reference.<br><span class="then">Optimise against it afterwards.</span>

Because it is the shortest path from a trained model to a correct result — and
correctness first is a good order to work in.

The export we have gives us both:

```
forward_stateful.pt      LibTorch, state inside the model
forward.onnx            ONNX, state in and out
```

Swapping the engine is a different `Engine` class behind the same three methods.

---

<div class="tag">What to take away</div>

## The choice of engine affects distribution,<br><span class="then">not only performance.</span>

<div class="statement-points">
  <div>Measure the size early — it is hard to walk back after a release</div>
  <div>Correct first, small second, fast third</div>
  <div>Keep the engine behind a thin interface, and the choice stays open</div>
</div>


Note:
    - Next: real-time safety, which no amount of size tuning fixes.
