// The dry/wet mix of an in-place processor: the dry signal, delayed by the
// latency the host is told, so it lines up with the wet one. Step 7's dry
// path, kept out of the way of the anira steps. Mono, as the model.
#pragma once

#include <cstddef>
#include <vector>

class DryWet {
public:
    // Allocates. Before the stream starts.
    // @latency: how late the wet signal is, in samples
    void prepare(size_t max_block_size, size_t latency) {
        m_line.assign(latency, 0.0f);
        m_dry.assign(max_block_size, 0.0f);
        m_position = 0;
    }

    // Before processing: keeps the block, delayed, as the dry signal. Audio thread.
    void store(const float* samples, size_t num_samples) {
        for (size_t i = 0; i < num_samples; ++i) {
            if (m_line.empty()) {
                m_dry[i] = samples[i];
                continue;
            }
            m_dry[i] = m_line[m_position];
            m_line[m_position] = samples[i];
            m_position = m_position + 1 < m_line.size() ? m_position + 1 : 0;
        }
    }

    // After processing: the wet block, mixed with the dry one. Audio thread.
    // @mix: 0 is dry only, 1 is wet only
    void mix(float* samples, size_t num_samples, float mix) const {
        for (size_t i = 0; i < num_samples; ++i) {
            samples[i] = mix * samples[i] + (1.0f - mix) * m_dry[i];
        }
    }

private:
    std::vector<float> m_line;  // The delay line
    std::vector<float> m_dry;   // This block's dry signal
    size_t m_position = 0;
};
