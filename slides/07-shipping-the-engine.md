<h1>Step 7<br>Shipping the engine</h1>

LibTorch, ONNX Runtime, or writing it yourself

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
- We ship the engine too: model 127 MB, engine can be bigger
- Which engine? How much work? How much control? How much size can we afford?
- No code in this step: a decision for your own product

**Script:**

"When we ship a plugin, we ship the engine too. Our model is 127 MB, and the engine can be bigger still. So we have to ask ourselves: Which engine do we pick? How much work do we want to do ourselves? How much control do we want? How much size can we afford? In this step we will not write any code. It's just a decision you'll have to make for your own product."

---

<div class="tag">Motivation</div>

## The problem<br><span class="then">You ship the model and the engine.</span>

Your own code is almost nothing:

```
your code     step01a_solution       203 KB
the model     model.pt               127 MB
the engine    libtorch runtime       318 MB
```

Note:
- What do we actually ship?
- Our own code is tiny: step 1a program, 203 KB
- Model file: 127 MB
- Engine, LibTorch: 318 MB, one file in it is already 206 MB
- Almost everything isn't our code
- All of it goes out with every install and every update

**Script:**

"Let's look at what we actually have to ship. Our own code is tiny: the program from step 1a is only 203 KB. The model file is 127 MB. And the engine, LibTorch, is 318 MB, and one single file in it is already 206 MB. So almost everything we ship isn't our code, it's the model and the engine. And all of it goes out with every install and every update."

---

<div class="tag">Motivation</div>

## Different engines, different sizes

macOS arm64, prebuilt, from `anira-project/backends` v2.4.0:

| engine | download | unpacked runtime |
|---|---|---|
| LibTorch 2.12 | 80 MB | **318 MB** |
| ONNX Runtime 1.26 | 6 MB | **24 MB** |
| ExecuTorch 1.3 (static) | 5 MB | — |
| LiteRT 2.1 | 3 MB | — |
| TFLite 2.17 | 2 MB | — |

Note:
- A model can often run on different engines, if they support its operators
- LibTorch and ONNX Runtime: same weights, match within 1e-6
- LibTorch 318 MB, ONNX Runtime 24 MB: 13× smaller
- LiteRT, ExecuTorch: a few MB to download, not measured unpacked

**Script:**

"A model can often run on different engines, as long as they support its operators. For our model, LibTorch and ONNX Runtime use the same weights and match to within 1e-6, but they have different performance characteristics and resource requirements. LibTorch unpacks to 318 MB, ONNX Runtime unpacks to 24 MB, which is 13 times smaller. LiteRT and ExecuTorch are only a few MB to download, but we haven't measured them unpacked."

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
- High-level: less work and maintenance: export, load, call
- Lower: smaller, more work, more control
- Balance size, work and control
- Top: LibTorch, PyTorch without Python, that's why it's big
- Bottom: by hand, small and potentially fast, but vectorisation, denormals and new architectures are your job

**Script:**

"But there is a trade-off for the size. The high-level engines will cost you less time to implement and maintain. You'll only have to export, load and call the model. The lower you go, the smaller it gets, but the more you'll have to do yourself. However, this also gives you more control. This means you have to balance the size of your engine with how much work you're willing to do and how much control you want. At the top end is LibTorch, which is really PyTorch without Python, and that's why it's so big. At the bottom end is writing the network by hand: small and potentially fast, but vectorisation, denormals and every new architecture are then your job."

---

<!-- .slide: data-visibility="hidden" -->

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
- Not really an inference engine: PyTorch without Python
- Benefits: the traced model is what runs, every operator and dtype, export-load-call
- Very large: autograd, JIT, quantisation, sparse tensors
- libtorch_cpu.dylib: one finished 206 MB file, loaded at start
- The linker only records "load this file": it can't strip what it can't see

**Script:**

"LibTorch isn't really an inference engine. It's PyTorch without Python, a complete deep learning framework in C++. The benefits are that the traced model is exactly what runs. You can use every operator, every dtype and you only have to export, load and call the model. However, it is a very large framework. It has autograd, JIT, quantisation, sparse tensors, things that you probably don't need at this stage of development. libtorch_cpu.dylib is one finished 206 MB file, loaded when the program starts. Your linker never looks inside it, it just records “load this file”. So the whole file ships, used or not used. The linker can't strip what it can't see."

---

<!-- .slide: data-visibility="hidden" -->

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
- Ask: who here has hand-written inference? What did it cost you?
- Custom implementation in C++ or similar: smallest, potentially fastest, a lot of work
- Potentially 0 MB, complete control, optimise for your use case
- Small networks (32-unit GRU, small TCN): a few hundred lines of C++
- But vectorisation, cache layout, denormals are yours: a project in itself

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

<p class="ships-note">Plus the model. Every user downloads that, on every install and every update.</p>

Note:
- Your users download this with your plugin
- Universal-binary VST3: LibTorch over 600 MB, model on top, every install and update
- Installers, notarisation, CDN bills, first impression of your product

**Script:**

"Remember your users download this when they download your plugin. In a universal-binary VST3, LibTorch is over
600 MB, and the model comes on top, on every install and every update. This might be a problem for your users. It's a lot of data to download and install. That's installers, notarisation, CDN bills, and the first impression of your product."

---

<div class="tag">First step</div>

## Start with LibTorch<br><span class="then">Optimise against it.</span>

**Good first step**: First LibTorch<br> **then** a lower-level engine.

For this workshop:

```
model.pt                LibTorch
model_forward.onnx      ONNX
```

Different engine class, but the same `process()` and `reset()`.

Note:
- Advice: start with LibTorch, it runs exactly what you traced, then optimise against it
- Here: exported both `.pt` and `.onnx`, checked against the same Python reference
- Different engine classes, same `process` and `reset`: the code around them doesn't change

**Script:**

"So, our advice is: start with LibTorch, because it runs exactly what you traced, then optimise against it. For this workshop we've exported both a `.pt` and an `.onnx`, and we check both against the same Python reference. They are different engine classes, but they have the same `process` and `reset`, so the code around them doesn't change."

---

