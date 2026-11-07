// The plugin shell steps 8 to 10 share, given complete: buses, the dry/wet
// parameter, a step's own knobs, the source player, the window, the bypass,
// the mono mix-down, and the host boilerplate. A step derives from it and
// writes three places: its constructor creates, prepare() prepares and
// returns the latency, process() runs one mono block.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
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

    // The host's calls, here once for every step: they call the step's
    // prepare(), process() and release() below.
    void prepareToPlay(double sample_rate, int samples_per_block) final;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) final;
    void releaseResources() final { release(); }

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
    // What a step writes, besides its constructor.
    //
    // Before the stream: the host's rate and its largest block. Allocate,
    // plan, start threads. May throw: the plugin is then bypassed, and the
    // window points to the console. Main thread.
    // @return the latency in samples, what the host is told
    virtual size_t prepare(double sample_rate, size_t max_block_size) = 0;
    // One callback, in place, on the input mixed down to mono; the result goes
    // out on every channel. Not while bypassed. Audio thread.
    virtual void process(float* samples, size_t num_samples) = 0;
    // After the stream, and before every prepare. Main thread.
    virtual void release() {}

    // The plugin does not process: the example or your file stops, the input
    // goes out unprocessed, and the window says why. A string literal, so this
    // is safe on the audio thread too. Each prepareToPlay starts without one.
    void bypass(const char* reason) { m_bypass.store(reason, std::memory_order_release); }
    bool bypassed() const { return m_bypass.load(std::memory_order_acquire) != nullptr; }

    // Step 8: built for 512-sample buffers at 44.1 kHz, the only settings its
    // prefill and latency are right for. Another buffer size or rate is
    // bypassed, and the window says why; 48 kHz asks the user first. Returns
    // whether the plugin can prepare. In prepare.
    bool fixed_settings_fit(double sample_rate, size_t max_block_size);

    // Steps 9 and 10: anira takes any rate, but the model is trained at 44.1
    // kHz: any other rate asks the user first. In prepare.
    void ask_unless_44k(double sample_rate);

    // After prepare: anira's plan report, one row per model. A row that is not
    // real-time holds the plugin back until run_anyway(); anira asks a live
    // host to do exactly that. Not on the audio thread.
    void set_report(std::vector<ReportRow> rows);

    // Whether this block goes through the model: not bypassed, and not held
    // back until the user agrees; a hold that is live only lets a bounce
    // (isNonRealtime) through. Audio thread.
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

    // The dry/wet position, 0 dry to 1 wet; dry only while the plugin is
    // held back. Audio thread.
    float mix() const { return processing() ? m_mix->load() : 0.0f; }

    // The value of the step's knob at `index`, in the order given. Audio thread.
    float knob(size_t index) const { return m_knob_values[index]->load(); }

private:
    // The example or your file replaces the input; the input stays as it is.
    // While the plugin does not process, the example or your file stops:
    // silence, and the player waits at the start, so it starts over once the
    // plugin processes again. The input still passes through. Audio thread.
    void render_source(juce::AudioBuffer<float>& buffer);

    // The model is mono: the input channels mixed down into the first one, in
    // place. Returns it. Audio thread.
    float* to_mono(juce::AudioBuffer<float>& buffer) const;

    // The first channel copied to every output channel. Audio thread.
    void to_all_channels(juce::AudioBuffer<float>& buffer) const;

    juce::String m_title;
    juce::String m_subtitle;
    std::vector<Knob> m_knobs;
    juce::AudioProcessorValueTreeState m_parameters;
    std::atomic<float>* m_mix = nullptr;
    std::vector<std::atomic<float>*> m_knob_values;
    SourcePlayer m_player;
    std::atomic<const char*> m_bypass{nullptr};
    // The two things that hold the plugin back, each on its own, both at once
    // if both apply: then the window asks for one, then for the other.
    enum class HoldKind { sample_rate, report };

    // The plugin waits for the user before it processes: the window shows
    // the hold and its button. A static Hold; nullptr releases that kind. A
    // hold that was not there yet asks again.
    void set_hold(HoldKind kind, const Hold* hold);
    bool waiting(size_t kind) const { return m_holds[kind].load() != nullptr && !m_agreed[kind].load(); }

    std::atomic<const Hold*> m_holds[2]{{nullptr}, {nullptr}};  // By HoldKind
    std::atomic<bool> m_agreed[2]{{false}, {false}};            // The user said yes to it
    std::atomic<uint32_t> m_report_version{0};
    mutable std::mutex m_report_mutex;  // prepareToPlay writes, the window reads
    std::vector<ReportRow> m_report;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBase)
};

// The host's entry point, for createPluginFilter(): the step's plugin. If its
// constructor throws (the model did not load, a file has an error), a plugin
// that only says so, instead of taking the host down.
juce::AudioProcessor* create_failed_plugin(const std::exception& error);

template <typename Plugin>
juce::AudioProcessor* create_plugin() {
    try {
        return new Plugin();
    } catch (const std::exception& error) {
        return create_failed_plugin(error);
    }
}
