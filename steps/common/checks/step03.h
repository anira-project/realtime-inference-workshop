// Step 3's checks, given complete. Not part of the exercise.
//
// Goal: say which TODO is off, from the prefill and from what the two ring
//       buffers hold after a callback.
#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

#include "common/helpers/ring_buffer.h"

// What the checks keep between prepare() and the callbacks.
struct Step03Count {
    size_t m_prefill = 0;  // The zeros m_output started with
    size_t m_arrived = 0;  // Samples the host gave since prepare()
};
inline Step03Count& step03_count() {
    static Step03Count count;
    return count;
}

// TODO 1, in prepare(): the host takes a block every callback, the model
// gives a whole model block back only once one is in. m_output starts with
// exactly the samples the host takes until then.
// @prefill: the zeros m_output starts with
// @host_block_size: the samples the host gives every callback, always the same
// @model_input_size: samples the model takes per forward pass
inline void check_prefill(size_t prefill, size_t host_block_size, size_t model_input_size) {
    const size_t needed = model_input_size - host_block_size;
    if (prefill < needed) {
        throw std::runtime_error(
            "TODO 1: the prefill is too small, the host would find m_output empty");
    }
    if (prefill > needed) {
        throw std::runtime_error(
            "TODO 1: the prefill is too large, the host would hear the model later than needed");
    }
    step03_count() = {prefill, 0};
}

// TODO 2 and 3, after a callback. No sample is made or lost on the way: each
// callback puts its samples into m_input and takes as many out of m_output,
// and the model moves a whole block from one to the other. So m_input holds
// what arrived since the last model block, and the two together hold the
// prefill.
// @input: the ring buffer in front of the model
// @output: the ring buffer behind the model
// @model_input_size: samples the model takes per forward pass
// @num_samples: the samples of this callback
inline void check_todos(const RingBuffer& input,
                        const RingBuffer& output,
                        size_t model_input_size,
                        size_t num_samples) {
    Step03Count& count = step03_count();
    count.m_arrived += num_samples;
    const size_t waiting = count.m_arrived % model_input_size;  // Since the last model block

    if (input.available() >= model_input_size) {
        throw std::runtime_error("TODO 2: run the model on the samples that arrived");
    }
    if (input.available() != waiting) {
        throw std::runtime_error(
            "TODO 2: m_input holds " + std::to_string(input.available()) + " samples, but " +
            std::to_string(waiting) + " arrived since the last model block: push every\n"
            "sample of the host, and run the model only once a whole block is in");
    }
    if (input.available() + output.available() < count.m_prefill) {
        throw std::runtime_error(
            "TODO 2: the model's result does not reach m_output: push it, all of it");
    }
    if (input.available() + output.available() > count.m_prefill) {
        throw std::runtime_error("TODO 3: give the host its samples back");
    }
}
