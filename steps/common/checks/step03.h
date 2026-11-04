// Step 3's checks, given complete. Not part of the exercise.
//
// Goal: say which TODO is still missing, from what the two ring buffers hold
//       after a callback.
#pragma once

#include <cstddef>
#include <stdexcept>

#include "common/helpers/ring_buffer.h"

// After a working callback the ring buffers have a capacity, samples are in
// flight, less than one model block waits for the model, and less than one
// model block waits for the host.
// @input: the ring buffer in front of the model
// @output: the ring buffer behind the model
// @model_input_size: samples the model takes per forward pass
inline void check_todos(const RingBuffer& input, const RingBuffer& output, size_t model_input_size) {
    if (input.available() + input.space() == 0) {
        throw std::runtime_error("TODO 1: give the ring buffers a capacity in prepare()");
    }
    if (input.available() == 0 && output.available() == 0) {
        throw std::runtime_error("TODO 2: push the samples into m_input and m_output");
    }
    if (input.available() >= model_input_size) {
        throw std::runtime_error("TODO 2: run the model on the samples that arrived");
    }
    if (output.available() >= model_input_size) {
        throw std::runtime_error("TODO 3: give the host its samples back");
    }
}
