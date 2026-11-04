# TODOs

- Teaser!
- Signs between tanh lab slide and scope shall be consistent..
- Change the audio example
- The integration process: Rectangle over Train and Export
- After QR-code -> slide how to clone and configure, also tell the audience to do it now!

- Onnxruntime minimal example first
- Then next process in an audio callback
- Shipping the engine, leave it out...

- Benchmark -> convert to samples

- All 512 hostsample examples
- synchronious ringbuffer example:
- Ringbuffersize input: 2048 and output: 2048
- Whats the latency? Bufferadaptation Latency: 2048-512 + Model latency (In this case ?)

- Async:
- Latency Bufferadaptation + Model Latency + inference_time_in_sample round up to next 512 samples (do not push model latency)
- Worker ohne sleep


- raus    // Wait for room, but do not hang at stop
    while (m_running.load(std::memory_order_acquire) &&
           m_output.space() < k_model.m_input_size) { sleep_for(100us); }

- Latency is collect a block, model inference (assumed one inference takes at max one buffersize of time), model internal latency (filters inside of the model, STFT, non-causal layers (i.e. convolutions), etc..)..
- In the case that the hostbuffersize is constant, is a Teiler von 2048 oder ein Multiple of 2048
- window not necessary
