// A fake host, given complete.
//
// Goal: hand your code the signal the way a DAW would — in blocks of the
//       host's choosing, not the model's. No audio device, so it also runs in
//       CI, and the block size is a parameter instead of a driver setting.
#pragma once

#include <cstddef>
#include <functional>
#include <vector>

namespace workshop {

// Calls `callback` with one block at a time until the signal is used up, in
// place: the callback overwrites what it was given.
// @signal: the input, copied so the caller keeps theirs
// @block_size: samples per callback — what the host decides, not you
// @callback: your processing, (samples, num_samples)
// Returns what the callback wrote, in order.
inline std::vector<float> run_host(const float* signal,
                                   size_t num_samples,
                                   size_t block_size,
                                   const std::function<void(float*, size_t)>& callback) {
    std::vector<float> audio(signal, signal + num_samples);

    for (size_t offset = 0; offset + block_size <= num_samples; offset += block_size) {
        callback(audio.data() + offset, block_size);
    }

    return audio;
}

}  // namespace workshop
