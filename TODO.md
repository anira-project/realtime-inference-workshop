# TODOs

- Signs between tanh lab slide and scope shall be consistent..
  - (Valentin) Not clear what is meant here: the icons (chip = Research there, Inference in C++ here), the layout, or something else?
- Change the audio example
  - (Valentin) Tried many, most sound really bad with this model. Keeping the current one for now, maybe a last-minute change.

- Benchmark -> convert to samples (todo-lina)

- Step 10 (encoder/decoder split) declares a fixed 512: `hard_host_blocks(k_host.m_block_size, k_host.m_block_size, …)`. Step 9 declares 1 to the host's maximum; decide whether the split should do the same, and update slides/16-split.md




- Latency is collect a block, model inference (assumed one inference takes at max one buffersize of time), model internal latency (filters inside of the model, STFT, non-causal layers (i.e. convolutions), etc..)..
- In the case that the hostbuffersize is constant, is a Teiler von 2048 oder ein Multiple of 2048
- window not necessary

- Blocking is also unsing exponential backoff at the beginning...
- An engine per channel?? in anira recap part 1 step 8

- Plans are not outputted at create but at prepare, check this again
- same with loaded models
- Have a look into the ctors and into prepare and please find out what the steps are really doing!!!!!!!
- Better handling of ... in   { "name": "audio_out", "role": "streamed", …, "latency": 2224 },
- We do not use the sleep anymore in the part 1 step 6 example..
- Add latency to blocking ratio plots
- Inline execution is next to come with gpu and npu accelerators
- Ab after process überarbeiten

- Better use bypass not zeros on miss in the example
- look at anchor tensors for the encoder decoder example!!
- Webcam on all plattforms!!!!!!!! Linux first!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
- Two models, two rates, one library. Wir haben 3 verschiedene Rates!
- Outlook at inline inference when buffersize matches and budget is fine and engine is declared real-time safe..

- Say it on the slides: part 1 only works with a fixed 512-sample host block (a divisor of 2048). anira handles any host block size, including the latency calculation, and also host blocks smaller than the announced maximum.
