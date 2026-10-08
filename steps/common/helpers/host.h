// A fake host, given complete.
//
// Goal: hand your code the signal the way a DAW would — in blocks of the
//       host's choosing, not the model's. No audio device, so it also runs in
//       CI, and the block size is a parameter instead of a driver setting.
#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <thread>
#include <vector>

// Calls `callback` with one block at a time until the signal is used up, in
// place: the callback overwrites what it was given.
// @signal: the input, copied so the caller keeps theirs
// @block_size: samples per callback — what the host decides, not you
// @callback: your processing, (samples, num_samples)
// @sample_rate: when set, the callbacks are spaced out as a device would space
//               them — needed as soon as another thread has to keep up
// Returns what the callback wrote, in order.
inline std::vector<float> run_host(const float* signal,
                                   size_t num_samples,
                                   size_t block_size,
                                   const std::function<void(float*, size_t)>& callback,
                                   double sample_rate = 0.0) {
    std::vector<float> audio(signal, signal + num_samples);

    const auto block_period = std::chrono::nanoseconds(
        sample_rate > 0.0
            ? static_cast<long long>(1e9 * static_cast<double>(block_size) / sample_rate)
            : 0);
    auto deadline = std::chrono::steady_clock::now();

    for (size_t offset = 0; offset + block_size <= num_samples; offset += block_size) {
        callback(audio.data() + offset, block_size);

        if (block_period.count() > 0) {
            deadline += block_period;
            std::this_thread::sleep_until(deadline);
        }
    }

    return audio;
}
