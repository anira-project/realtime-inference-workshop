// A ring buffer, given complete.
//
// Goal: hold samples between two sides that work in different block sizes. One
//       thread, fixed capacity, no allocation after construction.
//       Writing it is not the exercise; using it is.
#pragma once

#include <cstddef>
#include <vector>

class RingBuffer {
public:
    // @capacity: how many samples can be in flight at once
    explicit RingBuffer(size_t capacity) : m_samples(capacity, 0.0f) {}

    // Samples waiting to be read.
    size_t available() const { return m_size; }

    // Room left for writing.
    size_t space() const { return m_samples.size() - m_size; }

    // Appends one sample at the back. False, and nothing changes, if full.
    [[nodiscard]] bool try_push(float sample) {
        if (space() == 0) { return false; }
        m_samples[m_write] = sample;
        m_write = (m_write + 1) % m_samples.size();
        ++m_size;
        return true;
    }

    // Takes one sample from the front. False, and nothing changes, if empty.
    [[nodiscard]] bool try_pop(float& sample) {
        if (m_size == 0) { return false; }
        sample = m_samples[m_read];
        m_read = (m_read + 1) % m_samples.size();
        --m_size;
        return true;
    }

private:
    std::vector<float> m_samples;
    size_t m_read = 0;
    size_t m_write = 0;
    size_t m_size = 0;
};
