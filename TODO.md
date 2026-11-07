# TODOs

- I like the first teaser slide, but maybe we could have first the same slide with an fft, where it is deterministic and always fast enough. And then as a second example the model where there is not enough time...
- Process_size not copy in first example
- Don't get why we need the process_size? Can't we just put it directly when doing the engine.process?
- Was ist mit .clangd pointing to compile_commands.json symlink in build folder am besten und release and debug sollten am besten in unterordnern sein
- rename audio to output
- Benchmark fillout muss nach oben, rest muss in andere header file
- Worker thread grafik step 6 fixen
- Threadsafe Ringbuffers ohne one writer, one reader
- Model latency erst bei latency einbringen
- Latency einfach erklären ohne exercise
- Three places the host talks to us ersätzen
- Bypass nichts abspuielen
- Report, siehe anria example

## Old

- Signs between tanh lab slide and scope shall be consistent..
  - (Valentin) Not clear what is meant here: the icons (chip = Research there, Inference in C++ here), the layout, or something else?
- Change the audio example
  - (Valentin) Tried many, most sound really bad with this model. Keeping the current one for now, maybe a last-minute change.


## Done (2026-11-07)

- Why are there the quotes for stings in the path: step 1's TODO 1 was `model_path = ""`, now `model_path = ?`; the solution is `k_model.m_path`. Only the CMake define keeps its quotes, it makes the path a string literal.
- Why copy the floats in example 1? Why not bind tensors?: the tensors are made once, with memory from ONNX Runtime, and process() copies the block in and out. Binding the caller's buffer would mean a new Ort::Value per call (an allocation, RTSan would then report our engine in step 5) and, since process() works in place, still a copy back. The four copies of a call with state take about 7 µs here, the forward pass about 12 ms. anira binds its slot buffers and copies the audio in pre/post-processing; its state is two buffers that swap.
- OnnxEngine: process(float* audio, size_t num_samples, float* state, size_t state_size), the same shape for audio and state; no reset(), the stateless process() zero-fills the state input; no own buffers, create_tensor or elements().
- alle ringbuffer try_push and try_pop: RingBuffer and ThreadSafeRingBuffer have only try_push and try_pop (plus available, space), no block push/pop, no throws.
- Ringbuffer kürzer: ring_buffer.h without the block functions and their exceptions.
- has block raus / Bei ringbuffer has block in if cases: the check sits in the if, steps 3 and 5.
- Prefill muss in step 3 schon erwähnt werden: step 3's TODO 1 is the prefill, 2048 − 512.
- TODO 1 wird prefill und capacity wird automatisch berechnet: steps 3 and 6, the ring buffer sizes are given.
- If prefill is wrong assert: check_prefill throws "TODO 1: the prefill is too small/large" (asserts are off in release); the try_ calls that cannot fail are a bool plus assert.
- In step 3 ist wierd, dass man nicht die max_block_size benutzt: the prefill is k_model.m_input_size - max_block_size.
- TODO 3 bei threading raus, hot spinning muss gemacht werden: TODO 3 is the worker's hot spin on try_pop and try_push, with a running() check so stop() returns.
- Not consistent that some todos it is commented out like engine.process others not: every exercise uses ? placeholders.

## Done (2026-11-06)

- Ab after process überarbeiten: inline execution moved from Process into the Outlook, opened by the latency question and the step 4 deadline graph (512 ✗, 2048 ✓); the outlook also gets the restored "Which combination is fastest?" and a 4K zero-copy example. Slides corrected against alpha.3 (ABI 0.3, v2 shim, frames, Async, ring dtype, warm-up threads, pool lifetime).
- Benchmark -> convert to samples (Lina, bdfeb70 and 0f27866): step 4 prints and shows the worst case in samples and rounded to whole callbacks; k_max_inference_samples_rounded feeds steps 6 to 8.
- anira pinned to the v3.0.0-alpha.3 tag (ABI 0.3; anira-project/anira#241 merged: measured budget, until-stable warm-up, optional window, latency fixes, real-time promise per phase); an older pin in a build cache moves along. Steps 9, 10 and 11 run "budget": "measured" and "warmup": "until_stable", the checks print the measured budget.
- window not necessary: dropped from every model file of steps 9 to 11 and from the slides.
- anira latency fix: a state output no longer rounds the latency to whole model blocks (#235). Step 9 on my machine: 5295 instead of 6320 (measured 12.7 ms; exactly 512 → 4784, step 7's number). Slides and READMEs give the new numbers as this machine's.

## Done (2026-11-05)

- Bypass on a miss, as in anira's examples: step 9 (forward model) uses "bypass"; steps 10 and 11 keep "zeros" like anira's encoder/decoder example, since anira refuses bypass when output and input channels differ (8 latents from 1 audio channel). Step 9 README and the check output say "the miss policy's fill" instead of zeros.
- Webcam on all platforms: AVFoundation on macOS, JUCE on Windows, OpenCV on Linux (WORKSHOP_OPENCV_CAMERA, on by default on Linux); step 11 builds with the OpenCV camera on macOS too.
- Plans / loaded models at create / "find out what ctors and prepare really do" (730821d): create only checks the files and builds the slots and state pairs; prepare loads the models (pooled), builds the plans and starts the pool's threads. Checked by running anira: after create 0 plans and no new threads, after prepare 1 plan and 7 more threads, and a missing model file only fails at prepare. Map, create/prepare/architecture slides changed accordingly. Note: step 9's AniraProcessor creates the handler inside its prepare(), the slides say constructor.
- Blocking also uses exponential backoff (16b486b): BLOCKING does not use anira's backoff; moodycamel's semaphore spins up to 10,000 tries, then blocks on the OS semaphore (5 ms timeout only to check for exit). SPIN_BACKOFF: 4 retries, 32 with a CPU pause, then yield + 100 µs sleep. Waiting slide in 11-create updated.
- An engine per channel (77b0044, checked 7e99191): one engine per instance on a mono mix-down, everywhere.
- Latency = collect a block + inference + model-internal latency (e0404b6): steps 3 and 7 now say 2224 = 1696 from the decoder's transposed convolutions + 528 from the PQMF filter bank; step 7 notes list the general causes.
- Host block constant and a divisor of 2048 / "say it on the slides" (8815ef3): notes on step 3 "What is the problem?" and step 7 "A fixed latency".
- Async latency, worker without sleep, wait-for-room loop: already in the code (prefill = adaptation + rounded inference, model latency only reported; WorkerThread polls without sleep; output ring sized so the worker never waits for room, tested with a stalled worker).
- m_debt rewritten, check: Fares' if/else behaves like the one-liner; steps 6 and 7 pass.
- Part 2 latency for other host block sizes (45119aa, numbers since replaced by the measured 12.7 ms: 4784 / 5295): slides declare host blocks 1 up to the announced size; step 7 recap = 4784 for 512 only; new slide "Smaller blocks than announced" (4272 vs 6320); step 9 README explains 6320 by the 1…512 range (the 20 ms budget is a placeholder).
- All examples 512, ring buffers 2048/2048, "What's the latency?" bonus slide: already in step 3.
- Integration process: rectangle over Train and Export (18e29f9), plus a slide on the exported ONNX model after the audio example.
- After QR code: clone and configure slide (0c155e8), release preset with the reason in the notes.
- Shipping the engine: dropped by Lina in 72833ee.
- Teaser (9306635 and later): two animations, "Fighting the clock" (clock) and the real-time thread with an FFT next to a neural network.
