// What the plugin processes, given complete: the example, your own file, or
// the input. So the plugin has something to play without a DAW or a
// microphone.
//
// The example and your file play in a loop and replace the plugin's input. The
// input is passed through: the host's track, or in the standalone the
// microphone.
#pragma once

#include <BinaryData.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include <atomic>
#include <memory>

class SourcePlayer {
public:
    enum class Source { example, file, input };

    SourcePlayer() {
        m_formats.registerBasicFormats();
        read(m_example,
             std::make_unique<juce::MemoryInputStream>(
                 BinaryData::example_1_wav,
                 static_cast<size_t>(BinaryData::example_1_wavSize),
                 false));
    }

    // Reads a file from disk as your file. Message thread only, and the audio
    // thread must not be running: the processor suspends it around this.
    // Returns false if the file cannot be read; the old one stays.
    bool load(const juce::File& file) {
        if (!read(m_file, file.createInputStream())) { return false; }
        m_file_path = file;
        m_file_name = file.getFileName();
        m_position = 0;  // The old position may be past the new end
        return true;
    }

    void select(Source source) { m_source.store(source, std::memory_order_release); }
    Source selected() const { return m_source.load(std::memory_order_acquire); }
    bool has_file() const { return m_file_name.isNotEmpty(); }
    juce::String file_name() const { return m_file_name; }
    juce::File file() const { return m_file_path; }  // The last one loaded

    // The audio thread: overwrites the buffer with the example or your file,
    // looped. A mono file goes to every channel. The input is left as it is.
    void render(juce::AudioBuffer<float>& buffer) {
        const Source source = m_source.load(std::memory_order_acquire);
        if (source != m_current) {
            m_current = source;
            m_position = 0;
        }
        if (source == Source::input) { return; }

        const auto& samples = source == Source::example ? m_example : m_file;
        const int length = samples.getNumSamples();
        if (length == 0) {
            buffer.clear();
            return;
        }

        int position = 0;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            const float* in =
                samples.getReadPointer(std::min(channel, samples.getNumChannels() - 1));
            float* out = buffer.getWritePointer(channel);
            position = m_position;
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                out[i] = in[position];
                position = position + 1 < length ? position + 1 : 0;
            }
        }
        m_position = position;
    }

private:
    bool read(juce::AudioBuffer<float>& target, std::unique_ptr<juce::InputStream> stream) {
        if (stream == nullptr) { return false; }
        const std::unique_ptr<juce::AudioFormatReader> reader(
            m_formats.createReaderFor(std::move(stream)));
        if (reader == nullptr || reader->lengthInSamples <= 0) { return false; }

        target.setSize(static_cast<int>(reader->numChannels),
                       static_cast<int>(reader->lengthInSamples));
        reader->read(&target, 0, target.getNumSamples(), 0, true, true);
        return true;
    }

    juce::AudioFormatManager m_formats;
    juce::AudioBuffer<float> m_example;
    juce::AudioBuffer<float> m_file;
    juce::File m_file_path;    // Message thread
    juce::String m_file_name;  // Message thread
    std::atomic<Source> m_source{Source::example};
    Source m_current = Source::example;  // Audio thread only
    int m_position = 0;                  // Audio thread only
};
