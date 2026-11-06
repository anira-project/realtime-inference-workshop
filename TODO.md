# TODOs

- Signs between tanh lab slide and scope shall be consistent..
  - (Valentin) Not clear what is meant here: the icons (chip = Research there, Inference in C++ here), the layout, or something else?
- Change the audio example
  - (Valentin) Tried many, most sound really bad with this model. Keeping the current one for now, maybe a last-minute change.


- Ab after process überarbeiten


## Done (2026-11-06)

- Benchmark -> convert to samples (Lina, bdfeb70 and 0f27866): step 4 prints and shows the worst case in samples and rounded to whole callbacks; k_max_inference_samples_rounded feeds steps 6 to 8.
- anira pinned to the head of M2b PR 4 (anira-project/anira#241, 131b75de, alpha.2 + measured budget, until-stable warm-up, optional window, latency fixes); an old alpha.2 pin in a build cache moves along. Steps 9, 10 and 11 run "budget": "measured" and "warmup": "until_stable", the checks print the measured budget.
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
