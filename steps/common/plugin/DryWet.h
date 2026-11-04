// The dry/wet mix of an in-place processor: the dry signal, delayed by the
// latency the host is told, so it lines up with the wet one. Step 6's dry
// path, kept out of the way of the anira steps.
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <cstddef>
#include <vector>

class DryWet {
public:
    // Allocates. Before the stream starts.
    // @latency: how late the wet signal is, in samples
    void prepare(int num_channels, int max_block_size, size_t latency) {
        m_delay = latency;
        m_lines.assign(static_cast<size_t>(num_channels), std::vector<float>(latency, 0.0f));
        m_dry.setSize(num_channels, max_block_size);
        m_position = 0;
    }

    // Before processing: keeps the block, delayed, as the dry signal. Audio thread.
    void store(const juce::AudioBuffer<float>& buffer) {
        const int num_samples = buffer.getNumSamples();
        const auto channels = std::min(static_cast<size_t>(buffer.getNumChannels()), m_lines.size());
        size_t position = m_position;
        for (size_t channel = 0; channel < channels; ++channel) {
            const float* in = buffer.getReadPointer(static_cast<int>(channel));
            float* dry = m_dry.getWritePointer(static_cast<int>(channel));
            auto& line = m_lines[channel];
            position = m_position;
            for (int i = 0; i < num_samples; ++i) {
                if (m_delay == 0) {
                    dry[i] = in[i];
                    continue;
                }
                dry[i] = line[position];
                line[position] = in[i];
                position = position + 1 < m_delay ? position + 1 : 0;
            }
        }
        m_position = position;
    }

    // After processing: the wet block in the buffer, mixed with the dry one. Audio thread.
    // @mix: 0 is dry only, 1 is wet only
    void mix(juce::AudioBuffer<float>& buffer, float mix) const {
        const auto channels = std::min(static_cast<size_t>(buffer.getNumChannels()), m_lines.size());
        for (size_t channel = 0; channel < channels; ++channel) {
            float* out = buffer.getWritePointer(static_cast<int>(channel));
            const float* dry = m_dry.getReadPointer(static_cast<int>(channel));
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                out[i] = mix * out[i] + (1.0f - mix) * dry[i];
            }
        }
    }

private:
    size_t m_delay = 0;
    std::vector<std::vector<float>> m_lines;  // One delay line per channel
    juce::AudioBuffer<float> m_dry;           // This block's dry signal
    size_t m_position = 0;
};
