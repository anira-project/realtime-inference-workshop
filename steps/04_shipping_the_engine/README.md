# Step 4: Shipping the engine

**No exercise here** — this one is a talk and a decision. The numbers below come from this repository, so you can check them yourself.

**Goal:** know what the engine costs you at ship time, and what the alternatives trade away.

## What we ship right now

```
build/bin/step01_solution                     262 KB
build/_deps/workshop_libtorch-src/lib         318 MB
  libtorch_cpu.dylib                          206 MB
```

Check it yourself after a build:

```bash
ls -lh build/bin/step01_solution
du -sh build/_deps/workshop_libtorch-src/lib
```

## The same model on other engines

Prebuilt for macOS arm64, from the [anira-project/backends](https://github.com/anira-project/backends) v2.4.0 release:

| engine | download | unpacked runtime |
|---|---|---|
| LibTorch 2.12 | 80 MB | 318 MB |
| ONNX Runtime 1.26 | 6 MB | 24 MB |
| ExecuTorch 1.3 (static) | 5 MB | — |
| LiteRT 2.1 | 3 MB | — |
| TFLite 2.17 | 2 MB | — |

Same weights, same output to about 1e-6 — step 1 checks exactly that against the Python reference.

## Why LibTorch is the big one

LibTorch is PyTorch without Python, not a dedicated inference runtime: autograd, the full operator set, the JIT, quantisation, every kernel for every dtype. It all lives in one 206 MB shared object, so the linker cannot drop what your model does not use.

What you get for it: the thing you traced is the thing that runs, and the numerics match the training framework exactly.

## The trade-off

```
high level                                          low level
LibTorch ──────── ONNX Runtime ──────── LiteRT ──────── your own code
easiest            still easy            smaller         smallest
biggest            much smaller          leaner          fastest — if you are good
```

- **Higher level: less work.** Export, load, call.
- **Lower level: smaller and more controllable.** Op coverage becomes your problem.
- **Hand-written: smallest, and potentially fastest** — a GRU with 32 units or a small TCN is a few hundred lines of plain C++ with no dependency and no allocation. It is also the only option where every bug is yours, and every architecture change means writing it again. Fast hand-written code is a project, not a default.

## What this means for a plugin

The engine ships inside the bundle, per architecture. A universal VST3 is over 600 MB with LibTorch and around 50 MB with ONNX Runtime. That is what users download, what installers carry, and what gets notarised.

## Why we started with LibTorch anyway

It is the shortest path from a trained model to a correct result, and correct first is a good order to work in. The model is exported both ways:

```
models/forward_stateful.pt    LibTorch, state inside the model
forward.onnx                  ONNX, state in and out
```

Because the engine sits behind three methods — construct, `process`, `reset` — swapping it is a different class, not a different program.

## Slides

[`slides/04-shipping-the-engine.md`](../../slides/04-shipping-the-engine.md).

## What's next

Size is a shipping problem. The next step is a correctness problem that no amount of size tuning fixes: none of these engines is real-time safe.
