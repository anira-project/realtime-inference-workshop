// Step 3's checks, given complete. Not part of the exercise.
//
// Goal: say which TODO is off, from the prefill and from what the two ring
//       buffers hold after a callback.
#pragma once

#include <cstddef>
#include <stdexcept>

#include "common/helpers/ring_buffer.h"

// TODO 1, in prepare(): the host takes a block every callback, the model
// gives a whole model block back only once one is in. m_output starts with
// exactly the samples the host takes until then.
// @prefill: the zeros m_output starts with
// @max_block_size: the largest block the callback will be given
// @model_input_size: samples the model takes per forward pass
inline void check_prefill(size_t prefill, size_t max_block_size, size_t model_input_size) {
    const size_t needed = model_input_size - max_block_size;
    if (prefill < needed) {
        throw std::runtime_error(
            "TODO 1: the prefill is too small, the host would find m_output empty");
    }
    if (prefill > needed) {
        throw std::runtime_error(
            "TODO 1: the prefill is too large, the host would hear the model later than needed");
    }
}

// TODO 2 and 3, after a callback: less than one model block waits for the
// model, and less than one model block waits for the host.
// @input: the ring buffer in front of the model
// @output: the ring buffer behind the model
// @model_input_size: samples the model takes per forward pass
inline void check_todos(const RingBuffer& input, const RingBuffer& output, size_t model_input_size) {
    if (input.available() >= model_input_size) {
        throw std::runtime_error("TODO 2: run the model on the samples that arrived");
    }
    if (output.available() >= model_input_size) {
        throw std::runtime_error("TODO 3: give the host its samples back");
    }
}
