# Step 5: Real-time safety

**No exercise here** — this one is a demonstration. Build it, run it, read the report.

**Goal:** show that these engines are not real-time safe. Not "too slow": they allocate, take locks and call into the system, and any one of those can block for an unbounded time.

## What it does

`solution/main.cpp` calls the engine from a function marked `[[clang::nonblocking]]` — the promise an audio callback makes. [RealtimeSanitizer](https://clang.llvm.org/docs/RealtimeSanitizer.html) checks that promise at runtime and reports the first thing that breaks it.

```bash
./build/bin/step05_solution verify           # Both engines produce the reference output
./build/bin/step05_solution libtorch         # LibTorch, one block from the callback
./build/bin/step05_solution onnx             # ONNX Runtime, one block from the callback
./build/bin/step05_solution count libtorch   # Ten blocks, counted
./build/bin/step05_solution count onnx
```

Every mode runs in well under a second.

Both engines run the same model and agree with the Python reference:

```
  libtorch                 max abs diff 2.21e-06   OK
  onnx                     max abs diff 5.81e-07   OK
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
./build-rtsan/bin/step05_solution libtorch
```

Without `-DWORKSHOP_RTSAN=ON` the program still runs; it just checks nothing, and says so.

## What comes out

**LibTorch**, before the model even runs — this is `torch::from_blob`, the call that only *wraps* your buffer:

```
==81671==ERROR: RealtimeSanitizer: unsafe-library-call
Intercepted call to real-time unsafe function `malloc` in real-time context!
    #2 operator new(unsigned long)
    #3 c10::intrusive_ptr<c10::StorageImpl, ...>::make<...>
    #4 at::TensorMaker::make_tensor()
    #5 torch::from_blob(void*, c10::ArrayRef<long long>, ...)
    #6 LibTorchEngine::process(float*, unsigned long)
    #7 audio_callback<LibTorchEngine>(...)
```

**ONNX Runtime**, same place in the story — `CreateTensorWithDataAsOrtValue`, which also only wraps a buffer you already own:

```
==84091==ERROR: RealtimeSanitizer: unsafe-library-call
Intercepted call to real-time unsafe function `malloc` in real-time context!
    #3 OrtApis::CreateTensorWithDataAsOrtValue(...)
    #4 OnnxEngine::process(float*, unsigned long)
    #5 audio_callback<OnnxEngine>(...)
```

Neither report is about the forward pass. Both engines allocate *on the way in*, and there is no flag that turns that off. Behind those first calls are more allocations, an arena, thread pools and locks.

## Counting, over ten blocks

RTSan stops at the first violation, which says *that* it happens, not *how
often*. The counting mode runs ten callbacks and counts every allocation and
free the engine makes inside them — with its own `operator new`, so it works
without RTSan as well:

```
LibTorch, 10 blocks through the callback:
  block  0   591393 allocations and frees
  block  1    10960 allocations and frees
  ...
  total      690033 over 10 blocks, 69003 per block

ONNX Runtime, 10 blocks through the callback:
  block  9     1932 allocations and frees
  total       19321 over 10 blocks, 1932 per block
```

Three things to read out of that:

- **Eleven thousand allocations per callback** for LibTorch, two thousand for ONNX Runtime. Not one slip — the engines are built that way.
- **Block 0 costs 591,393**, fifty times a normal block, although the engine was warmed up before. TorchScript recompiles while it runs. That is the 84 ms worst case from step 3, with a cause.
- **ONNX Runtime is an order of magnitude leaner** and still unusable here.

RTSan can list what kind of violations they are, if you tell it not to stop at
the first one:

```bash
RTSAN_OPTIONS=halt_on_error=false ./build-rtsan/bin/step05_solution onnx 2>&1 >/dev/null \
  | grep -oE 'unsafe function `[a-z_]+`' | sort | uniq -c | sort -rn
```

One block through ONNX Runtime:

```
  98 malloc      51 free      10 pthread_mutex_lock      10 pthread_mutex_unlock
```

The same for LibTorch: 4,393 × `malloc`, 3,103 × `free`, 265 × `posix_memalign`, 180 × `pthread_mutex_lock`/`unlock`. Locks, on the audio thread.

Keep this to a single block: RTSan symbolizes every report, so one LibTorch block takes about 20 seconds, and the counting mode above is the fast way to see the scale.

## Why you cannot fix this from the outside

- The allocation is inside the engine, in a shared library you link against.
- A custom allocator helps with *where* the memory comes from, not with *whether* a lock is taken.
- Running the engine with one thread, as the ONNX engine here does, removes the thread pool but not the allocations.
- Preallocating on your side (as `OnnxEngine` does for its input vector) moves your own allocations out of the way, and the engine's remain.

The conclusion is not "tune the engine". It is: **inference does not belong on the audio thread.**

## Slides

[`slides/05-realtime-safety.md`](../../slides/05-realtime-safety.md).

## What's next

If it cannot run on the audio thread, it has to run on another one — and then the audio thread and that thread have to exchange data without locking. That is the next step.
