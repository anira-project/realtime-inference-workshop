# TODOs

- Remove the Claude trailer from commit 72833ee (Lina, 2026-11-04, "Renumber part 1 to steps 1-8 and drop the shipping step"): `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`, the only one in the history. Rewrites the 70 commits after it and needs a force push of main; Lina resets her main to origin/main afterwards.


## Old

- Signs between tanh lab slide and scope shall be consistent..
  - (Valentin) Not clear what is meant here: the icons (chip = Research there, Inference in C++ here), the layout, or something else?
- Change the audio example
  - (Valentin) Tried many, most sound really bad with this model. Keeping the current one for now, maybe a last-minute change.


## Done (2026-11-07)

- Bypass nichts abspuielen: while bypassed, the example or your file stops (silence, back to the start) and starts over once the settings fit; the input still passes through. Step 8 processes short blocks instead of bypassing on them.
- Report, siehe anira example: the plugins of steps 9 and 10 keep anira's PlanReport row per model; a **report** button opens a panel (verdict, budget, measured, median, warm-up, load bar with the 0.75 and 1.0 thresholds). A not-real-time plan holds the plugin back live, with **Run anyway, with dropouts**; a bounce always processes. The report comes from the handler (plan_report()), the exercise code is untouched; the checks of steps 9 and 10 print it, and step 9 has a slide for it.
- Worker thread grafik step 6 fixen: the two-lane graphic (step 6 motivation and the part 2 recap) is a timeline of 12 callbacks, every row aligned: callbacks, m_input filling, the worker spinning with a model run after every fourth, m_output starting with the 5 prefill blocks, what the host hears; a playhead sweeps over it ({{THREADS}} in scripts/build.js). The points below: its own clock, its own rules.
- Model latency erst bei latency einbringen / Latency einfach erklären ohne exercise: step 3's bonus slide asks for the algorithmic latency only, 2048 − 512 = 1536, the prefill. Step 7 is a demonstration, solution only (exercise and its filled check gone): its slides bring in the model's own 2224 through the dry/wet mix, the dry path delayed by the whole latency.
- Threadsafe Ringbuffers ohne one writer, one reader: the table slide in step 6 became one flow animation, "Every callback 512 in, 512 out", in two lanes: the callback's host block (not a ring) into m_input, the worker spinning on try_pop until a model block is whole, the model, the result into m_output, the audio thread taking the prefill first. One writer, one reader is a line below it and in the notes.
- Benchmark fillout muss nach oben, rest muss in andere header file: inference_time.h holds only k_max_inference_ms. Step 4 prints and shows milliseconds only; the 46.4 ms check stays below it. Step 6 introduces the worker's time: an explainer slide, and `worker_time` computed in two given lines at the top of prepare(); steps 7 and 8 compute it the same way.
- rename audio to output: the signal processed in place is `buffer` in every step (1, 2, 3, 5, 6, 7), in the checks of steps 1 and 2 and in host.h; the slides follow. The engine's parameter stays `audio`.
- Exercises: every blank is a ?, in steps 1–3, 6, 7, 9 and 10, C++ and model files; step 9's commented-out calls are real lines now. The exercises stay out of the default build, scripts/fill_exercise.py fills each from its solution (stepNN_exercise_filled, step09/10_exercise_filled_check), and CI runs them: an exercise committed filled in or drifted from its solution fails, naming the line.
- Process_size not copy in first example / Don't get why we need the process_size?: step 1 has two TODOs, `OnnxEngine engine(?)` and `engine.process(?, ?)`; no model_path or process_size copies, no check_todos. A wrong size still gets the engine's "the model takes exactly 2048 samples per call".
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
- Teaser A first with an FFT, then the model: renamed "Every callback has a deadline", A = the clock with FFT + IFFT (about 3 µs with FFTW, roughly 0.1 samples; one box per block shows the time per block), B = the same clock with the network; the codec teaser (FFT and network lanes) dropped.
- Exported to ONNX slide simplified: "Shipped as an ONNX file" (Open Neural Network Exchange), the file = architecture + weights; export pipeline, sizes and download details dropped.
- Build folders: the presets build into build/release/ and build/debug/ (step 5's RTSan build into build/rtsan/); every configure links build/compile_commands.json to its tree (cmake/compile-commands.cmake, as in anira), .clangd reads build/. Slides and READMEs use the new paths.
- Step 11's hand tracker on anira v3 (it ran on compat/v2.hpp): one model config (the v3 documents the shim generated) and Hard contract per model, one context; the decoder and the tracker share hands::context_config() (ContextSettings.h: one thread per core, blocking, errors only), so no log-level, wait-strategy or v2-upgrade warnings. Unused parts dropped: the synchronous engine, backend switching. Checked: v2 and v3 paths give bit-identical outputs, latencies and result counts for all four models.
