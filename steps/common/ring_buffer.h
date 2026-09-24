// A ring buffer, given complete.
//
// Goal: hold samples between two sides that work in different block sizes. One
//       writer, one reader, fixed capacity, no allocation after construction.
//       Writing it is not the exercise; using it is.
#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace workshop {

class RingBuffer {
public:
    // @capacity: how many samples can be in flight at once
    explicit RingBuffer(size_t capacity) : m_samples(capacity, 0.0f) {}

    // Samples waiting to be read.
    size_t available() const { return m_size; }

    // Room left for writing.
    size_t space() const { return m_samples.size() - m_size; }

    // Appends num_samples. Throws if they do not fit — in a plugin you would
    // size the buffer so this cannot happen, which is the point of TODO 1.
    void push(const float* samples, size_t num_samples) {
        if (num_samples > space()) {
            throw std::runtime_error("ring buffer overflow: " + std::to_string(num_samples) +
                                     " samples pushed, room for " + std::to_string(space()));
        }
        for (size_t i = 0; i < num_samples; ++i) {
            m_samples[m_write] = samples[i];
            m_write = (m_write + 1) % m_samples.size();
        }
        m_size += num_samples;
    }

    // Removes num_samples into `samples`. Throws if that many are not there.
    void pop(float* samples, size_t num_samples) {
        if (num_samples > m_size) {
            throw std::runtime_error("ring buffer underflow: " + std::to_string(num_samples) +
                                     " samples popped, only " + std::to_string(m_size) + " there");
        }
        for (size_t i = 0; i < num_samples; ++i) {
            samples[i] = m_samples[m_read];
            m_read = (m_read + 1) % m_samples.size();
        }
        m_size -= num_samples;
    }

private:
    std::vector<float> m_samples;
    size_t m_read = 0;
    size_t m_write = 0;
    size_t m_size = 0;
};

}  // namespace workshop
