# Step 5: Real-time safety

**No exercise here** — this one is a demonstration. Build it, run it, read the report.

**Goal:** show that the engine is not real-time safe. Not "too slow": they allocate, take locks and call into the system, and any one of those can block for an unbounded time.

## What it does

`solution/main.cpp` calls the engine from a function marked `[[clang::nonblocking]]` — the promise an audio callback makes. [RealtimeSanitizer](https://clang.llvm.org/docs/RealtimeSanitizer.html) checks that promise at runtime and reports the first thing that breaks it.

```bash
./build/bin/step05_solution          # One block from the callback
./build/bin/step05_solution verify   # The engine produces the reference output
./build/bin/step05_solution count    # Ten blocks, counted
```

Every mode runs in well under a second.

The engine runs the model of the previous steps and agrees with the Python reference:

```
  onnx                     max abs diff 3.87e-07   OK
```

## Building with RTSan

RTSan needs a clang that has it — on macOS the Homebrew LLVM, not Apple Clang:

```bash
brew install llvm
cmake -S . -B build-rtsan -DCMAKE_BUILD_TYPE=Release \
  -DWORKSHOP_RTSAN=ON \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)" \
  -DWORKSHOP_STEP=05
cmake --build build-rtsan -j
./build-rtsan/bin/step05_solution
```

Without `-DWORKSHOP_RTSAN=ON` the program still runs; it just checks nothing, and says so.

## What comes out

`OrtApis::Run`, on its way into the model. Our engine creates its tensors once, up front, and hands `Run` the outputs too, and it still allocates:

```
==57170==ERROR: RealtimeSanitizer: unsafe-library-call
Intercepted call to real-time unsafe function `malloc` in real-time context!
    #1 operator new(unsigned long)
    #2 OrtApis::Run(...)
    #3 Ort::detail::SessionImpl<OrtSession>::Run(...)
    #4 OnnxEngine::run(float*, unsigned long)
    #5 OnnxEngine::process(float*, unsigned long, std::vector<float>&)
    #6 audio_callback(OnnxStream&, float*, unsigned long)
```

The report is not about the forward pass. The engine allocates *on the way in*, and there is no flag that turns that off. Behind that first call are more allocations, an arena, thread pools and locks.

## Counting, over ten blocks

RTSan stops at the first violation, which says *that* it happens, not *how
often*. The counting mode runs ten callbacks and counts every allocation and
free the engine makes inside them — with its own `operator new`, so it works
without RTSan as well:

```
ONNX Runtime, 10 blocks through the callback:
  block  9     2154 allocations and frees
  total       21541 over 10 blocks, 2154 per block
```

**Two thousand allocations and frees per callback**, every block. Not one slip — the engine is built that way.

RTSan can list what kind of violations they are, if you tell it not to stop at
the first one:

```bash
RTSAN_OPTIONS=halt_on_error=false ./build-rtsan/bin/step05_solution 2>&1 >/dev/null \
  | grep -oE 'unsafe function `[a-z_]+`' | sort | uniq -c | sort -rn
```

One block through ONNX Runtime:

```
  82 malloc      38 free      8 pthread_mutex_lock      8 pthread_mutex_unlock
```

Locks, on the audio thread.

Keep this to a single block: RTSan symbolizes every report, and the counting mode above is the fast way to see the scale.

## Why you cannot fix this from the outside

- The allocation is inside the engine, in a shared library you link against.
- A custom allocator helps with *where* the memory comes from, not with *whether* a lock is taken.
- Running the engine with one thread, as the ONNX engine here does, removes the thread pool but not the allocations.
- Preallocating on your side (as `OnnxEngine` does for its tensors and buffers) moves your own allocations out of the way, and the engine's remain.

The conclusion is not "tune the engine". It is: **inference does not belong on the audio thread.**

## Slides

[`slides/05-realtime-safety.md`](../../slides/05-realtime-safety.md).

## What's next

If it cannot run on the audio thread, it has to run on another one — and then the audio thread and that thread have to exchange data without locking. That is the next step.
