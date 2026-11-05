// The ring buffer from step 3, safe between two threads. Given complete.
//
// Goal: move samples between the audio thread and the worker (step 6). One
//       writer, one reader, fixed capacity, no lock, no allocation after
//       construction.
//
// Same functions as step 3's RingBuffer, but none of them waits or throws: on
// the audio thread neither is allowed. Underneath is moodycamel's
// single-producer single-consumer circular buffer, used only through its try_
// functions, which never wait and never allocate.
#pragma once

#include <readerwritercircularbuffer.h>

#include <cstddef>

class ThreadSafeRingBuffer {
public:
    // @capacity: how many samples can be in flight at once
    explicit ThreadSafeRingBuffer(size_t capacity) : m_samples(capacity) {}

    // Samples waiting to be read. Seen from the reader's thread this can only
    // be too small, never too large, so checking it before pop() is safe.
    size_t available() const { return m_samples.size_approx(); }

    // Appends num_samples. Returns false if they did not all fit: the rest is
    // dropped. Only one thread may push.
    bool push(const float* samples, size_t num_samples) {
        for (size_t i = 0; i < num_samples; ++i) {
            if (!m_samples.try_enqueue(samples[i])) { return false; }
        }
        return true;
    }

    // Removes num_samples into `samples`. Returns false, and takes nothing, if
    // fewer are available: check available() first. Only one thread may pop.
    bool pop(float* samples, size_t num_samples) {
        if (num_samples > available()) { return false; }
        for (size_t i = 0; i < num_samples; ++i) { m_samples.try_dequeue(samples[i]); }
        return true;
    }

    // One sample at a time, for the audio thread: false, and nothing changes,
    // if full or empty.
    bool try_push(float sample) { return m_samples.try_enqueue(sample); }
    bool try_pop(float& sample) { return m_samples.try_dequeue(sample); }

private:
    moodycamel::BlockingReaderWriterCircularBuffer<float> m_samples;
};
