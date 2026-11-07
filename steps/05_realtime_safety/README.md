# Step 5: Real-time safety

**No exercise here** — this one is a demonstration. Build it, run it, read the report.

**Goal:** show that the engine is not real-time safe. Not "too slow": it allocates, takes locks and calls into the system, and any one of those can block for an unbounded time.

## What it does

[`solution/main.cpp`](solution/main.cpp) is step 3's processor, with its callback, `process_block`, marked `[[clang::nonblocking]]` — the promise an audio callback makes. [RealtimeSanitizer](https://clang.llvm.org/docs/RealtimeSanitizer.html) checks that promise at runtime and reports the first thing that breaks it.

The host loop is step 3's. Without RTSan nothing is checked, so the program warns, runs through, and shows that the engine still matches the Python reference:

```
  NOTE  built without RealtimeSanitizer, so real-time safety is not checked. It needs
        clang >= 20 from LLVM (macOS: Homebrew LLVM, not Apple Clang; or Linux) and
        -DWORKSHOP_RTSAN=ON. See the README.
  PASS  the output matches the reference: max abs diff 3.87e-07, tolerance 0.0001

==============================================================
  STEP 5 SOLVED, but real-time safety is not checked: no RealtimeSanitizer
==============================================================
```

## Building with RTSan

RTSan needs a clang that has it — on macOS the Homebrew LLVM, not Apple Clang:

```bash
brew install llvm
cmake -S . -B build/rtsan -DCMAKE_BUILD_TYPE=Release \
  -DWORKSHOP_RTSAN=ON \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)" \
  -DWORKSHOP_STEP=05
cmake --build build/rtsan -j
./build/rtsan/bin/step05_solution
```

## What comes out

The run stops in the fourth callback, the first one that runs the model, in `OrtApis::Run`, on its way into the model. Our engine creates its tensors once, up front, and hands `Run` the outputs too, and it still allocates:

```
==64590==ERROR: RealtimeSanitizer: unsafe-library-call
Intercepted call to real-time unsafe function `malloc` in real-time context!
    #2 operator new(unsigned long)
    #3 OrtApis::Run(...)
    #4 OnnxEngine::run(float*, unsigned long)
    #5 OnnxEngine::process(float*, unsigned long, float*, unsigned long)
    #6 ProcessorExample::process_block(float*, unsigned long)
    #7 main
```

The first report comes on the way into the forward pass, before the model has run, and it's not the last: the engine keeps allocating during the pass, layer by layer. There is no flag that turns that off.

## Every violation, not just the first

RTSan stops at the first violation, which says *that* it happens, not *how often*. Tell it not to stop, and to report a call stack again even when it has seen it before, and it reports every one, for the whole test signal: 32 callbacks, 8 forward passes.

```bash
RTSAN_OPTIONS=halt_on_error=false:suppress_equal_stacks=false \
  ./build/rtsan/bin/step05_solution 2>&1 >/dev/null \
  | grep -oE 'unsafe function `[a-z_]+`' | sort | uniq -c | sort -rn
```

```
9019 unsafe function `malloc`
9014 unsafe function `free`
1164 unsafe function `pthread_mutex_unlock`
1164 unsafe function `pthread_mutex_lock`
```

About 1,130 allocations, almost as many frees and 145 locks in every forward pass. Not one slip — the engine is built that way. Locks, on the audio thread.

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
