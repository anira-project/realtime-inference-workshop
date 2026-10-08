<h1>Step 4<br>Shipping the engine</h1>

LibTorch, ONNX Runtime, or writing it yourself

<!-- .slide: data-state="no-header" -->
<!-- kind: talk -->

Note:
    - No exercise here. Talk, numbers, and a decision the room has to make for their own product.

---

## What we have been shipping

Our step 1 binary:

```
step01_solution          262 KB
```

What has to sit next to it:

```
libtorch runtime         318 MB
  libtorch_cpu.dylib     206 MB   ← one file
```

A 262 KB program and 318 MB of engine.

Note:
    - Measured on this machine, macOS arm64, from the anira-project/backends v2.4.0 release.

---

## The same model, other engines

macOS arm64, prebuilt, from `anira-project/backends` v2.4.0:

| engine | download | unpacked runtime |
|---|---|---|
| LibTorch 2.12 | 80 MB | **318 MB** |
| ONNX Runtime 1.26 | 6 MB | **24 MB** |
| ExecuTorch 1.3 (static) | 5 MB | — |
| LiteRT 2.1 | 3 MB | — |
| TFLite 2.17 | 2 MB | — |

Same weights, same output to 1e-6. **13× smaller** for the ONNX path.

---

## Why LibTorch is that big

It is not an inference engine. It is **PyTorch**, minus Python:

- Autograd, the whole operator set, JIT, quantisation, sparse tensors
- Every kernel for every dtype, whether your model uses it or not
- `libtorch_cpu.dylib` is one 206 MB object; the linker cannot drop what it cannot see

Convenient, because the thing you traced is exactly the thing that runs.

---

## The trade-off

```
high level                                          low level
LibTorch ──────── ONNX Runtime ──────── LiteRT ──────── your own code
easiest            still easy            smaller         smallest
biggest            much smaller          leaner          fastest — if you are good
```

- **More high-level: less work.** Export, load, call. The graph decides.
- **More low-level: smaller, more control.** You carry the op coverage problem.
- **Hand-written: smallest and potentially fastest** — and the only option where
  you own every bug.

---

## When hand-written wins

A GRU with 32 units, a small TCN, a biquad-shaped network: a few hundred lines of
plain C++, no dependency, no allocation, no graph.

**But:** vectorisation, cache layout, denormals, and every architecture change
means writing it again. Fast code is not the default — it is a project.

Note:
    - RTNeural as the known example. Ask who in the room has done it, and what it cost them.

---

## What a plugin actually ships

A VST3 is a bundle. Your engine goes inside it, per architecture:

| path | universal binary |
|---|---|
| LibTorch | > 600 MB |
| ONNX Runtime | ~ 50 MB |
| hand-written | ~ 0 |

Users download that. Installers, notarisation, CDN bills, and the first
impression of your product.

---

## So why did we start with LibTorch?

Because it is the shortest path from a trained model to a correct result — and
correctness first is a good order to work in.

The export we have gives us both:

```
forward_stateful.pt      LibTorch, state inside the model
forward.onnx            ONNX, state in and out
```

Swapping the engine is a different `Engine` class behind the same three methods.

---

## What to take away

- The engine is a **shipping decision**, not only a performance one
- Measure the size early — it is hard to walk back from after a release
- Correct first, small second, fast third
- Keep the engine behind a thin interface, and the choice stays open

<!-- .slide: data-state="no-footer" -->

Note:
    - Next: real-time safety, which no amount of size tuning fixes.
