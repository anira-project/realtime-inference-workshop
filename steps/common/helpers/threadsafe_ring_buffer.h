// The ring buffer from step 2, safe between two threads. Given complete.
//
// Goal: the same push, pop, available and space as RingBuffer, with the writer
//       on one thread and the reader on another (step 5). One writer, one
//       reader, fixed capacity, no lock, no allocation after construction.
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
    // One sample more underneath: see space().
    explicit ThreadSafeRingBuffer(size_t capacity) : m_samples(capacity + 1) {}

    // Samples waiting to be read. From the writer's thread this can only be
    // too small, never too large: checking it before pop() is safe.
    size_t available() const { return m_samples.size_approx(); }

    // Room left for writing. While the reader takes a sample out, the count
    // has already dropped but the slot is not free yet; keeping one sample in
    // reserve makes sure this, too, is never too large.
    size_t space() const { return m_samples.max_capacity() - 1 - available(); }

    // Appends num_samples to the back of the buffer. Throws if they do not fit:
    // check space() first. Only one thread may push.
    void push(const float* samples, size_t num_samples) {
        if (num_samples > space()) {
            throw std::runtime_error("ring buffer overflow: " + std::to_string(num_samples) +
                                     " samples pushed, room for " + std::to_string(space()));
        }
        for (size_t i = 0; i < num_samples; ++i) { m_samples.try_enqueue(samples[i]); }
    }

    // Removes num_samples from the front of the buffer into `samples`. Throws if
    // there are not enough: check available() first. Only one thread may pop.
    void pop(float* samples, size_t num_samples) {
        if (num_samples > available()) {
            throw std::runtime_error("ring buffer underflow: " + std::to_string(num_samples) +
                                     " samples popped, only " + std::to_string(available()) + " available");
        }
        for (size_t i = 0; i < num_samples; ++i) { m_samples.try_dequeue(samples[i]); }
    }

private:
    moodycamel::BlockingReaderWriterCircularBuffer<float> m_samples;
};
