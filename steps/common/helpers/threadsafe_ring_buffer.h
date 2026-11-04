// The ring buffer from step 2, safe between two threads. Given complete.
//
// Goal: move samples between the audio thread and the worker (step 5). One
//       writer, one reader, fixed capacity, no lock, no allocation after
//       construction.
//
// Underneath is moodycamel's single-producer single-consumer circular buffer.
// Only its try_ functions are used: they never wait and never allocate.
#pragma once

#include <readerwritercircularbuffer.h>

#include <cstddef>
#include <stdexcept>
#include <string>

class ThreadSafeRingBuffer {
public:
    // @capacity: how many samples can be in flight at once
    explicit ThreadSafeRingBuffer(size_t capacity) : m_samples(capacity) {}

    // Appends one sample. Returns false, and changes nothing, if the buffer is
    // full. Only one thread may push.
    bool try_push(float sample) { return m_samples.try_enqueue(sample); }

    // Takes the oldest sample out. Returns false, and leaves `sample` alone, if
    // the buffer is empty. Only one thread may pop.
    bool try_pop(float& sample) { return m_samples.try_dequeue(sample); }

    // Samples waiting to be read. Seen from the reader's thread this can only
    // be too small, never too large, so checking it before pop() is safe.
    size_t available() const { return m_samples.size_approx(); }

    // Appends num_samples. Throws if they do not all fit — used to prefill the
    // buffer before the threads start, where that would be a sizing mistake.
    void push(const float* samples, size_t num_samples) {
        for (size_t i = 0; i < num_samples; ++i) {
            if (!m_samples.try_enqueue(samples[i])) {
                throw std::runtime_error("ring buffer overflow: room for " + std::to_string(i) +
                                         " of " + std::to_string(num_samples) + " samples");
            }
        }
    }

    // Removes num_samples into `samples`. Throws if there are not enough:
    // check available() first. Only one thread may pop.
    void pop(float* samples, size_t num_samples) {
        if (num_samples > available()) {
            throw std::runtime_error("ring buffer underflow: " + std::to_string(num_samples) +
                                     " samples popped, only " + std::to_string(available()) +
                                     " available");
        }
        for (size_t i = 0; i < num_samples; ++i) { m_samples.try_dequeue(samples[i]); }
    }

private:
    moodycamel::BlockingReaderWriterCircularBuffer<float> m_samples;
};
