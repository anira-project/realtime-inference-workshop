// A file player with three slots, so the plugin has something to play without a
// DAW or a microphone. The slots start with the examples in assets/, compiled
// into the plugin; any slot can be replaced with a file from disk.
//
// While a slot plays, it replaces the plugin's input. The model then hears the
// file, and the dry/wet mix compares the file with what the model made of it.
#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

#include <BinaryData.h>

#include <array>
#include <atomic>
#include <memory>

class ExamplePlayer {
public:
    static constexpr int k_num_slots = 3;

    ExamplePlayer() {
        m_formats.registerBasicFormats();

        const std::array<std::pair<const char*, int>, k_num_slots> embedded = {{
            {BinaryData::example_1_wav, BinaryData::example_1_wavSize},
            {BinaryData::example_2_wav, BinaryData::example_2_wavSize},
            {BinaryData::example_3_wav, BinaryData::example_3_wavSize},
        }};
        for (int slot = 0; slot < k_num_slots; ++slot) {
            const auto& [data, size] = embedded[static_cast<size_t>(slot)];
            read(slot,
                 std::make_unique<juce::MemoryInputStream>(data, static_cast<size_t>(size), false),
                 "example_" + juce::String(slot + 1) + ".wav");
        }
    }

    // Replaces a slot with a file from disk. Message thread only, and the
    // audio thread must not be running: the processor suspends it around this.
    // Returns false if the file cannot be read; the slot keeps what it had.
    bool load(int slot, const juce::File& file) {
        return read(slot, file.createInputStream(), file.getFileName());
    }

    // @slot: the slot to play from the start, or -1 to stop
    void play(int slot) { m_requested.store(slot, std::memory_order_release); }

    int playing() const { return m_requested.load(std::memory_order_acquire); }

    juce::String name(int slot) const { return m_names[static_cast<size_t>(slot)]; }

    // The audio thread: overwrites the buffer with the playing slot, looped, if
    // one is playing. A mono file goes to every channel.
    void render(juce::AudioBuffer<float>& buffer) {
        const int slot = m_requested.load(std::memory_order_acquire);
        if (slot != m_current) {
            m_current = slot;
            m_position = 0;
        }
        if (slot < 0) { return; }

        const auto& source = m_slots[static_cast<size_t>(slot)];
        const int length = source.getNumSamples();
        if (length == 0) {
            buffer.clear();
            return;
        }

        int position = 0;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            const float* in = source.getReadPointer(std::min(channel, source.getNumChannels() - 1));
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
    bool read(int slot, std::unique_ptr<juce::InputStream> stream, const juce::String& name) {
        if (stream == nullptr) { return false; }
        const std::unique_ptr<juce::AudioFormatReader> reader(
            m_formats.createReaderFor(std::move(stream)));
        if (reader == nullptr || reader->lengthInSamples <= 0) { return false; }

        auto& target = m_slots[static_cast<size_t>(slot)];
        target.setSize(static_cast<int>(reader->numChannels),
                       static_cast<int>(reader->lengthInSamples));
        reader->read(&target, 0, target.getNumSamples(), 0, true, true);
        m_names[static_cast<size_t>(slot)] = name;
        if (m_current == slot) { m_position = 0; }  // The old position may be past the new end
        return true;
    }

    juce::AudioFormatManager m_formats;
    std::array<juce::AudioBuffer<float>, k_num_slots> m_slots;
    std::array<juce::String, k_num_slots> m_names;
    std::atomic<int> m_requested{-1};
    int m_current = -1;  // Audio thread only
    int m_position = 0;  // Audio thread only
};
