// Step 7 — Latency and the dry/wet mix
//
// Goal:   work out how late the processed signal is, report it, and delay the
//         dry signal by the same amount so the two can be mixed.
// Given:  the threaded processor from step 6, with a dry path added.
// Task:   fill in the two TODO banners.
// Check:  at 100% dry the output is the input, delayed by exactly the reported
//         latency — if the number is wrong, the check says so.
//
// It builds and runs as it is, and says which TODO is still open.

#include <readerwriterqueue.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#include "common/helpers/host.h"
#include "common/helpers/libtorch_engine.h"
#include "common/helpers/ring_buffer.h"
#include "common/helpers/support.h"
#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"

// Model settings taken from the export metadata.
constexpr struct {
    const char* m_path = WORKSHOP_MODEL_PATH;
    size_t m_input_size = 2048;
    double m_sample_rate = 48000.0;
} k_model{};

constexpr size_t k_host_block_size = 512;  // What the host hands over per callback
constexpr size_t k_late_capacity = 1024;   // Late callbacks remembered for the report
constexpr size_t k_queue_capacity = 8;     // Model blocks in flight between the threads

namespace {

struct ModelBlock {
    std::array<float, 2048> m_samples{};
};

#if defined(__has_cpp_attribute) && __has_cpp_attribute(clang::nonblocking)
#define WORKSHOP_AUDIO_CALLBACK [[clang::nonblocking]]
#else
#define WORKSHOP_AUDIO_CALLBACK
#endif

// Step 6's processor, with a dry path and a mix control.
class LatencyProcessor {
public:
    explicit LatencyProcessor(LibTorchEngine& engine) : m_engine(engine) {}

    ~LatencyProcessor() { stop(); }
    
    // ---- TODO 1: how late is the wet signal? ---------------------------------
    // Two things delay it, and both are fixed by the design:
    //   - the model cannot run before a whole block has arrived
    //   - the result is only picked up in a later callback, so the output side
    //     starts out primed with a block of silence
    // What the host is told has to be what the host gets, so this number also
    // decides how far the dry signal is delayed, and how much silence both
    // paths start with in prepare().
    // --------------------------------------------------------------------------
    size_t latency_samples() const { return 0; }


    // Allocate and initialize all processing buffers, queue capacity, 
    // the thread before audio processing begins.
    // @max_block_size: the largest block process_block() can be given
    void prepare(size_t max_block_size) {
        stop();

        m_input = RingBuffer(max_block_size + k_model.m_input_size);
        m_output = RingBuffer(max_block_size + k_queue_capacity * k_model.m_input_size);
        m_dry = RingBuffer(max_block_size + latency_samples());
        m_engine.reset();
        m_dry_block.assign(max_block_size, 0.0f);
        m_wet_block.assign(max_block_size, 0.0f);
        m_wet_debt = 0;
        m_position = 0;
        m_late.clear();
        m_late.reserve(k_late_capacity);

        // Prime both paths with the same amount of silence: one block while the
        // input is still being collected, and one more so the worker has a full
        // block of time to answer. That is what makes the latency a fixed
        // number instead of whatever the worker happened to manage.
        m_silence.assign(latency_samples(), 0.0f);
        m_output.push(m_silence.data(), m_silence.size());
        m_dry.push(m_silence.data(), m_silence.size());

        m_to_worker = moodycamel::ReaderWriterQueue<ModelBlock>(k_queue_capacity);
        m_from_worker = moodycamel::ReaderWriterQueue<ModelBlock>(k_queue_capacity);

        m_running.store(true, std::memory_order_release);
        m_worker = std::thread([this] { worker(); });
    }

    void stop() {
        if (!m_worker.joinable()) { return; }
        m_running.store(false, std::memory_order_release);
        m_worker.join();
    }

    // @mix: 0 is dry only, 1 is wet only
    void set_mix(float mix) { m_mix = mix; }

    // The audio thread: samples in, samples out, nothing that can block.
    void process_block(float* samples, size_t num_samples) WORKSHOP_AUDIO_CALLBACK {
        // The dry side is only this full when nobody takes anything out of it,
        // which means the mix below is still missing. No exception here: the
        // audio thread does not throw.
        if (m_dry.available() > latency_samples()) {
            m_mix_missing.store(true, std::memory_order_relaxed);
            return;
        }

        m_input.push(samples, num_samples);
        m_dry.push(samples, num_samples);

        while (m_input.available() >= k_model.m_input_size) {
            ModelBlock block;
            m_input.pop(block.m_samples.data(), k_model.m_input_size);

            // Queue full means the worker is behind. The block is already out
            // of the ring buffer, so it is dropped — a glitch, but a bounded
            // one. Keeping it would let the backlog grow without end.
            if (!m_to_worker.try_enqueue(block)) {
                m_dropped.fetch_add(1, std::memory_order_relaxed);
            }
        }

        ModelBlock done;
        while (m_output.space() >= k_model.m_input_size && m_from_worker.try_dequeue(done)) {
            m_output.push(done.m_samples.data(), k_model.m_input_size);
        }

        // A block that came too late already went out as silence. Drop it when
        // it arrives, so the wet path stays exactly latency_samples() late: a
        // late block is a glitch, not a shift for the rest of the stream.
        while (m_wet_debt > 0 && m_output.available() > 0) {
            const size_t n = std::min({m_wet_debt, m_output.available(), m_wet_block.size()});
            m_output.pop(m_wet_block.data(), n);
            m_wet_debt -= n;
        }
        const bool wet_ready = m_output.available() >= num_samples;

        // ---- TODO 2 ----------------------------------------------------------
        // Produce exactly num_samples for the host by mixing the delayed dry
        // path with the wet path.
        //   - Read the dry samples from m_dry into m_dry_block.
        //   - Read wet samples into m_wet_block only when wet_ready is true.
        //   - For each sample, apply m_mix: 0 means fully dry, 1 means fully wet.
        //   - Store the mixed result in samples.
        // Neither side is guaranteed to have samples ready, and the host
        // gets num_samples either way.
        // ----------------------------------------------------------------------
        std::fill_n(samples, num_samples, 0.0f);


        // The wet side had nothing for this callback: its share is owed.
        if (!wet_ready) {
            m_wet_debt += num_samples;
            if (m_late.size() < m_late.capacity()) { m_late.push_back(m_position); }
        }
        m_position += num_samples;
    }

    const std::vector<float>& produced() const { return m_produced; }

    // Where a late wet block went out as silence, in samples from the start.
    const std::vector<size_t>& late() const { return m_late; }

    // True when the dry path filled up instead of being mixed and handed over.
    bool mix_missing() const { return m_mix_missing.load(std::memory_order_relaxed); }

private:
    void worker() {
        ModelBlock block;
        while (m_running.load(std::memory_order_acquire)) {
            if (!m_to_worker.try_dequeue(block)) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }
            m_engine.process(block.m_samples.data(), k_model.m_input_size);
            m_produced.insert(m_produced.end(), block.m_samples.begin(), block.m_samples.end());
            // Wait for room, but keep watching the stop flag: the audio thread
            // may have stopped calling us, and then this would never return —
            // a lock-free queue does not save you from a deadlock at shutdown.
            while (!m_from_worker.try_enqueue(block)) {
                if (!m_running.load(std::memory_order_acquire)) { break; }
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
        while (m_to_worker.try_dequeue(block)) {
            m_engine.process(block.m_samples.data(), k_model.m_input_size);
            m_produced.insert(m_produced.end(), block.m_samples.begin(), block.m_samples.end());
        }
    }

    LibTorchEngine& m_engine;
    RingBuffer m_input{0};
    RingBuffer m_output{0};
    RingBuffer m_dry{0};
    moodycamel::ReaderWriterQueue<ModelBlock> m_to_worker{0};
    moodycamel::ReaderWriterQueue<ModelBlock> m_from_worker{0};
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    std::atomic<long> m_dropped{0};  // Blocks the worker could not take in time
    std::vector<float> m_produced;
    std::vector<float> m_silence;
    std::vector<float> m_dry_block;  // Scratch, sized in prepare()
    std::vector<float> m_wet_block;
    size_t m_wet_debt = 0;  // Wet samples whose slot already went out as silence
    size_t m_position = 0;  // Samples handed to the host so far
    std::vector<size_t> m_late;  // Where a late wet block went out as silence
    float m_mix = 1.0f;
    std::atomic<bool> m_mix_missing{false};
};

// Runs the whole signal through the processor at one mix setting.
// @late: set to where a late wet block went out as silence
std::vector<float> run(LatencyProcessor& processor, float mix, std::vector<size_t>& late) {
    processor.prepare(k_host_block_size);
    processor.set_mix(mix);

    std::vector<float> output = run_host(
        k_input_signal.data(),
        k_input_signal.size(),
        k_host_block_size,
        [&processor](float* samples, size_t num_samples) {
            processor.process_block(samples, num_samples);
        },
        k_model.m_sample_rate);

    processor.stop();
    late = processor.late();
    return output;
}

// Compares a signal against a reference that is `latency` samples earlier.
// Callbacks where a late wet block went out as silence are left out: they are
// glitches, and what matters is that everything around them still lines up.
// @got: what came out of the processor
// @want: what it should be, undelayed
// @latency: how far `want` has to be pushed back to line up
// @late: where a late wet block went out as silence, in callback order
float diff_with_latency(const std::vector<float>& got, const float* want, size_t latency,
                        const std::vector<size_t>& late) {
    if (got.size() <= latency) { return 1.0f; }
    float diff = 0.0f;
    size_t next = 0;
    for (size_t i = latency; i < got.size(); ++i) {
        while (next < late.size() && i >= late[next] + k_host_block_size) { ++next; }
        if (next < late.size() && i >= late[next]) { continue; }
        diff = std::max(diff, std::abs(got[i] - want[i - latency]));
    }
    return diff;
}

}  // namespace

int main() {
    std::unique_ptr<LibTorchEngine> engine;
    try {
        engine = std::make_unique<LibTorchEngine>(k_model.m_path);
    } catch (const std::runtime_error& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    LatencyProcessor processor(*engine);
    const size_t latency = processor.latency_samples();

    if (latency == 0) {
        std::printf("TODO 1: work out the latency the design introduces.\n");
        return 1;
    }

    std::printf("Reported latency: %zu samples (%.1f ms at %.0f Hz)\n\n",
                latency,
                1000.0 * static_cast<double>(latency) / k_model.m_sample_rate,
                k_model.m_sample_rate);

    std::vector<size_t> dry_late;
    std::vector<size_t> wet_late;
    const std::vector<float> dry = run(processor, 0.0f, dry_late);
    if (processor.mix_missing()) {
        std::printf("TODO 2: mix dry and wet, and hand the result to the host.\n");
        return 1;
    }

    const std::vector<float> wet = run(processor, 1.0f, wet_late);

    const float dry_diff = diff_with_latency(dry, k_input_signal.data(), latency, {});
    const float wet_diff = diff_with_latency(wet, k_target_output_signal.data(), latency, wet_late);

    const bool dry_ok = report_line("dry, delayed by the latency", dry_diff);
    const bool wet_ok = report_line("wet, delayed by the latency", wet_diff);

    // A slow or busy machine can make the worker late now and then; those
    // callbacks are silent and left out above. Late most of the time is a
    // machine that cannot run the model in real time at all.
    const size_t callbacks = k_input_signal.size() / k_host_block_size;
    if (!wet_late.empty()) {
        std::printf("  late wet callbacks, silent    %zu of %zu\n", wet_late.size(), callbacks);
    }
    if (wet_late.size() * 4 > callbacks) {
        std::printf("\nFAILED: the worker was late in more than a quarter of the callbacks.\n");
        return 1;
    }

    if (dry_ok && wet_ok) {
        std::printf("\nOK: both paths are late by exactly the reported latency, so they mix.\n");
        return 0;
    }
    std::printf("\nFAILED: the reported latency does not match what comes out.\n");
    return 1;
}
