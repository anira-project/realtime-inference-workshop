#pragma once

#include <array>
#include <atomic>
#include <cstddef>

// Lock-free single-writer / single-reader latest-value hand-off; writer and reader can never collide on the same slot.
namespace anira::models::handtracking {

template <typename T>
class TripleBuffer {
public:
    T& writeBuffer() { return m_slots[static_cast<size_t>(m_write_index)]; }

    void publish() {  // writer thread
        const int previous =
            m_ready.exchange(m_write_index | k_fresh_bit, std::memory_order_acq_rel);
        m_write_index = previous & k_index_mask;
    }

    const T& read() {  // reader thread
        if ((m_ready.load(std::memory_order_acquire) & k_fresh_bit) != 0) {
            const int previous = m_ready.exchange(m_read_index, std::memory_order_acq_rel);
            m_read_index = previous & k_index_mask;
        }
        return m_slots[static_cast<size_t>(m_read_index)];
    }

private:
    static constexpr int k_fresh_bit = 4;
    static constexpr int k_index_mask = 3;

    std::array<T, 3> m_slots{};
    std::atomic<int> m_ready{2};
    int m_write_index = 0;  // writer-owned
    int m_read_index = 1;   // reader-owned
};

}  // namespace anira::models::handtracking
