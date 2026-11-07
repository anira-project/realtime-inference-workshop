// The ring buffer from step 3, safe between two threads. Given complete.
//
// Goal: move samples between the audio thread and the worker (step 6). One
//       writer, one reader, fixed capacity, no lock, no allocation after
//       construction.
//
// One sample at a time, and nothing waits: try_push() and try_pop() say
// whether it worked. The audio thread gives up, the worker tries again.
// Underneath is moodycamel's single-producer single-consumer circular buffer.
#pragma once

#include <readerwritercircularbuffer.h>

#include <cstddef>

class ThreadSafeRingBuffer {
public:
    // @capacity: how many samples can be in flight at once
    explicit ThreadSafeRingBuffer(size_t capacity) : m_samples(capacity) {}

    // Samples waiting to be read. Seen from the reader's thread this can only
    // be too small, never too large: that many try_pop() calls will work.
    size_t available() const { return m_samples.size_approx(); }

    // For the audio thread: never wait. False, and nothing changes, if full
    // or empty.
    [[nodiscard]] bool try_push(float sample) { return m_samples.try_enqueue(sample); }
    [[nodiscard]] bool try_pop(float& sample) { return m_samples.try_dequeue(sample); }

private:
    moodycamel::BlockingReaderWriterCircularBuffer<float> m_samples;
};
