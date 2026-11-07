// The plugin shell steps 8 to 10 share, given complete: buses, the dry/wet
// parameter, a step's own knobs, the source player, the window, and the host
// boilerplate. A step derives from it and writes the places where the host
// talks to it: prepareToPlay, processBlock, releaseResources, and the latency
// it reports.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

#include "SourcePlayer.h"

class PluginBase : public juce::AudioProcessor {
public:
    // One row of anira's plan report (steps 9 and 10), one per model: what
    // prepare measured on this machine. Plain values, so step 8 needs no anira.
    struct ReportRow {
        enum class Verdict { unmeasured, comfortable, marginal, not_realtime };

        juce::String m_name;  // "model", "encoder", "decoder"
        Verdict m_verdict = Verdict::unmeasured;
        double m_budget_ms = 0.0;    // What anira plans with, per inference
        double m_measured_ms = 0.0;  // The warm-up's maximum of its last inferences
        double m_median_ms = 0.0;
        uint32_t m_warmup_inferences = 0;
        juce::String m_warmup;     // How the warm-up ended: stable, capped, ...
        double m_rt_factor = 0.0;  // Inference time over the audio it produces
        uint32_t m_rt_parallel = 1;  // Inferences that run at once
        // Below 0.75 comfortable, below 1 marginal, else not real-time
        double load() const { return m_rt_parallel > 0 ? m_rt_factor / m_rt_parallel : m_rt_factor; }
    };

    // One of a step's own parameters, a slider in a row of its own.
    struct Knob {
        const char* m_id;  // Also the slider's label
        float m_min;
        float m_max;
        float m_default;
    };

    // @title, @subtitle: the window's header
    // @knobs: the step's own parameters, besides dry/wet
    PluginBase(juce::String title, juce::String subtitle, std::vector<Knob> knobs = {});
    ~PluginBase() override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    // For the window. Message thread.
    juce::AudioProcessorValueTreeState& parameters() { return m_parameters; }
    SourcePlayer& player() { return m_player; }
    const juce::String& title() const { return m_title; }
    const juce::String& subtitle() const { return m_subtitle; }
    const std::vector<Knob>& knobs() const { return m_knobs; }
    // Why the plugin holds back until the user agrees: the window's title,
    // the reason, and the button that agrees.
    struct Hold {
        const char* m_title;
        const char* m_reason;
        const char* m_button;
        bool m_live_only;  // An offline render (a bounce) is not held back
    };
    // anira measured a plan slower than real time (steps 9 and 10): live it
    // drops out; a bounce waits for every result, so it is not held back.
    static constexpr Hold k_not_realtime{
        "Not real-time",
        "anira measured this model slower than real time on this machine: live, it would drop "
        "out, and you would hear clicks. Run anyway plays it nonetheless.",
        "Run anyway, with dropouts",
        true};
    // The host's rate is not the model's 44.1 kHz (steps 9 and 10): the model
    // plays at the wrong speed, so it sounds higher or lower. Step 8 has its own.
    static constexpr Hold k_not_44k{
        "44.1 kHz only",
        "The model is trained at 44.1 kHz: at another rate it sounds higher or lower. A "
        "resampler would fix it; this plugin has none. Process anyway plays it as it is.",
        "Process anyway",
        false};
    // The two things that hold the plugin back, each on its own, both at once
    // if both apply: then the window asks for one, then for the other.
    enum class HoldKind { sample_rate, report };

    // Why the plugin is bypassed, or nullptr. A bypass goes before a hold.
    const char* bypass_reason() const { return m_bypass.load(std::memory_order_acquire); }
    // The holds the user has not agreed to yet, the sample rate first. The
    // window asks for the first one. Message thread.
    std::vector<const Hold*> holds() const;
    // A hold the user has not agreed to yet.
    bool held_back() const { return waiting(0) || waiting(1); }
    // The user agrees to the first hold that waits, the one the window shows:
    // kept until that hold comes back after it was gone.
    void run_anyway();
    // anira's plan report from the last prepare, empty without one (step 8).
    // Message thread.
    std::vector<ReportRow> report() const;
    // Counts up with every new report, so the window knows when to repaint.
    uint32_t report_version() const { return m_report_version.load(); }
    // Reads a file as your file and plays it, with the audio thread held off
    // while it changes. Returns false if it cannot be read.
    bool load_file(const juce::File& file);

protected:
    // The plugin does not process: the example or your file stops, the input
    // goes out unprocessed, and the window says why. A string literal, so this
    // is safe on the audio thread too.
    void bypass(const char* reason) { m_bypass.store(reason, std::memory_order_release); }
    void process_normally() { m_bypass.store(nullptr, std::memory_order_release); }
    bool bypassed() const { return m_bypass.load(std::memory_order_acquire) != nullptr; }

    // After prepare: anira's plan report, one row per model. A row that is not
    // real-time holds the plugin back (k_not_realtime) until run_anyway();
    // anira asks a live host to do exactly that. Not on the audio thread.
    void set_report(std::vector<ReportRow> rows);

    // The plugin waits for the user before it processes: the window shows
    // the hold and its button. A static Hold; nullptr releases that kind. A
    // hold that was not there yet asks again. In prepareToPlay.
    void set_hold(HoldKind kind, const Hold* hold);

    // Whether this block goes through the model: not bypassed, and not held
    // back; a hold that is live only lets a bounce (isNonRealtime) through.
    // Audio thread.
    bool processing() const {
        if (bypassed()) { return false; }
        if (!held_back()) { return true; }
        if (!isNonRealtime()) { return false; }
        for (size_t kind = 0; kind < 2; ++kind) {
            const Hold* hold = m_holds[kind].load();
            if (hold != nullptr && !m_agreed[kind].load() && !hold->m_live_only) { return false; }
        }
        return true;  // Only live holds wait: a bounce goes through
    }

    // The example or your file replaces the input; the input stays as it is.
    // While the plugin does not process, the example or your file stops:
    // silence, and the player waits at the start, so it starts over once the
    // plugin processes again. The input still passes through. Audio thread.
    void render_source(juce::AudioBuffer<float>& buffer) {
        // A channel the host gave no input (a mono track into a stereo output)
        // holds what was there before, often our own last output: it gets the
        // input too, or silence without one, before the dry path keeps it.
        const int inputs = getTotalNumInputChannels();
        for (int channel = inputs; channel < buffer.getNumChannels(); ++channel) {
            if (inputs > 0) {
                buffer.copyFrom(channel, 0, buffer, 0, 0, buffer.getNumSamples());
            } else {
                buffer.clear(channel, 0, buffer.getNumSamples());
            }
        }

        if (!processing() && m_player.selected() != SourcePlayer::Source::input) {
            buffer.clear();
            m_player.rewind();
            return;
        }
        m_player.render(buffer);
    }

    // The model is mono: the input channels mixed down into the first one, in
    // place. Returns it. Audio thread.
    float* to_mono(juce::AudioBuffer<float>& buffer) const;

    // The first channel copied to every output channel. Audio thread.
    void to_all_channels(juce::AudioBuffer<float>& buffer) const;

    // The dry/wet position, 0 dry to 1 wet. Audio thread.
    float mix() const { return m_mix != nullptr ? m_mix->load() : 1.0f; }

    // The value of the step's knob at `index`, in the order given. Audio thread.
    float knob(size_t index) const { return m_knob_values[index]->load(); }

private:
    juce::String m_title;
    juce::String m_subtitle;
    std::vector<Knob> m_knobs;
    juce::AudioProcessorValueTreeState m_parameters;
    std::atomic<float>* m_mix = nullptr;
    std::vector<std::atomic<float>*> m_knob_values;
    SourcePlayer m_player;
    std::atomic<const char*> m_bypass{nullptr};
    bool waiting(size_t kind) const { return m_holds[kind].load() != nullptr && !m_agreed[kind].load(); }

    std::atomic<const Hold*> m_holds[2]{{nullptr}, {nullptr}};  // By HoldKind
    std::atomic<bool> m_agreed[2]{{false}, {false}};            // The user said yes to it
    std::atomic<uint32_t> m_report_version{0};
    mutable std::mutex m_report_mutex;  // prepareToPlay writes, the window reads
    std::vector<ReportRow> m_report;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBase)
};
