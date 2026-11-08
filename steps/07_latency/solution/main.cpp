// Step 7 — Latency and the dry/wet mix
//
// Goal:   work out how late the processed signal is, report it, and delay the
//         dry signal by the same amount so the two can be mixed.
// Check:  in the mix, dry and wet line up: both late by exactly the reported
//         latency.

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <thread>
#include <vector>

#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"
#include "common/checks/step07.h"
#include "common/helpers/host.h"
#include "common/helpers/onnx_engine.h"
#include "common/helpers/ring_buffer.h"
#include "common/helpers/threadsafe_ring_buffer.h"
#include "common/helpers/worker_thread.h"
#include "common/inference_time.h"

// Model settings taken from the export metadata, as in step 1.
// @m_latency: the model's own delay, how much later its output answers its input
constexpr struct ModelConfig {
    const char* m_path = WORKSHOP_ONNX_MODEL_PATH;
    size_t m_input_size = 2048;
    size_t m_state_size = 63488;
    double m_sample_rate = 44100.0;
    size_t m_latency = 2224;
} k_model{};

// What the host hands over per callback: four callbacks make one model block.
constexpr size_t k_host_block_size = 512;

// The mix the host sets: half dry, half wet.
constexpr float k_mix = 0.5f;

// Step 6's processor, with a dry path next to it:
//
//   audio thread:   host block -> m_input              m_output -> wet --+
//   worker thread:                m_input -> model -> m_output           mix -> host block
//   audio thread:   host block -> m_dry ---------------------------> dry --+
//
// m_dry is a plain RingBuffer: only the audio thread uses it.
class ProcessorExample {
public:
    // Keeps the model and makes the engine, which loads nothing yet:
    // prepare() loads the model.
    explicit ProcessorExample(const ModelConfig& model) : m_model(model) {}

    void prepare(size_t host_block_size, double max_inference_ms) {
        m_worker.stop();

        // The model, loaded the first time: it takes a while and allocates.
        if (!m_engine.loaded()) { m_engine.load(m_model.m_path); }

        // Step 6's worker time: your worst case, in samples, in whole callbacks
        const double max_inference_samples = max_inference_ms * m_model.m_sample_rate / 1000.0;
        const size_t rounded_max_inference_samples =
            static_cast<size_t>(std::ceil(max_inference_samples / host_block_size)) * host_block_size;
        // The buffer adaptation, step 3: the samples it takes until the worker
        // has its first whole block
        const size_t adaptation = m_model.m_input_size - host_block_size;
        m_prefill = adaptation + rounded_max_inference_samples;
        const size_t output_capacity = rounded_max_inference_samples + m_model.m_input_size;

        // The latency: how late the wet signal is, the number the host is told;
        // in a plugin it goes to setLatencySamples(). Three things delay it, and
        // all are fixed:
        //   - the buffer adaptation (step 3)
        //   - the time the worker is given (step 6)
        //   - the model itself answers late: m_model.m_latency, from the export
        m_latency = m_prefill + m_model.m_latency;

        m_input = ThreadSafeRingBuffer(m_model.m_input_size);
        m_output = ThreadSafeRingBuffer(output_capacity);
        for (size_t i = 0; i < m_prefill; ++i) {
            const bool pushed = m_output.try_push(0.0f);  // The prefill
            assert(pushed);  // never fails: m_output has room for the prefill
        }
        m_model_block.assign(m_model.m_input_size, 0.0f);
        m_state.assign(m_model.m_state_size, 0.0f);

        // The dry path: room for the delay and one callback more.
        m_dry = RingBuffer(m_latency + host_block_size);
        m_dry_block.assign(host_block_size, 0.0f);

        // The dry delay: m_dry starts with the whole latency in silence, so dry
        // and wet line up. The model's own delay is already inside what the
        // worker returns; the dry path has nothing like that.
        const size_t dry_prefill = m_latency;
        for (size_t i = 0; i < dry_prefill; ++i) {
            const bool pushed = m_dry.try_push(0.0f);
            assert(pushed);  // never fails: m_dry has room for the delay
        }

        std::fill(m_state.begin(), m_state.end(), 0.0f);
        m_unpushed = 0;
        m_debt = 0;

        // From here on, process_model() runs over and over on the worker thread.
        m_worker.start([this] { process_model(); });
    }

    void stop() { m_worker.stop(); }

    // @mix: 0 is dry only, 1 is wet only
    void set_mix(float mix) { m_mix = mix; }

    // What the host is told: how late the output is, in samples.
    size_t latency_samples() const { return m_latency; }

    // The audio thread: one callback. No engine anymore, only samples in and out.
    void process_block(float* samples, size_t num_samples) {
        // The dry path: in, and out as late as m_dry was prefilled.
        for (size_t i = 0; i < num_samples; ++i) {
            const bool pushed = m_dry.try_push(samples[i]);
            assert(pushed);  // never fails: room for the delay and one callback more
            const bool popped = m_dry.try_pop(m_dry_block[i]);
            assert(popped);  // never fails: m_dry starts with the delay
        }

        for (size_t i = 0; i < num_samples; ++i) {
            const bool pushed = m_input.try_push(samples[i]);
            if (!pushed) {
                m_unpushed += num_samples - i;
                break;
            }
        }

        const size_t zeros = std::min(m_unpushed, num_samples);
        std::fill_n(samples, zeros, 0.0f);
        m_unpushed -= zeros;

        float late = 0.0f;
        while (m_debt > 0) {
            if (m_output.try_pop(late)) {
                --m_debt;
            } else {
                break;
            }
        }

        for (size_t i = zeros; i < num_samples; ++i) {
            const bool popped = m_debt == 0 && m_output.try_pop(samples[i]);
            if (!popped) {
                // Not the next one either: while anything is owed, or while
                // the worker is pushing right now, what comes next would land
                // one place too early.
                std::fill_n(samples + i, num_samples - i, 0.0f);
                m_debt += num_samples - i;
                break;
            }
        }

        // Dry and wet are equally late now, so the mix is a plain crossfade.
        for (size_t i = 0; i < num_samples; ++i) {
            samples[i] = (1.0f - m_mix) * m_dry_block[i] + m_mix * samples[i];
        }
    }

private:
    // The worker thread: called over and over. The engine may allocate and
    // lock here, nobody is waiting on this thread.
    void process_model() {
        // Step 6's hot spin: try again right away until a whole model block
        // is in, and until the result is out.
        for (size_t i = 0; i < m_model.m_input_size; ++i) {
            while (!m_input.try_pop(m_model_block[i])) {
                if (!m_worker.running()) { return; }  // so stop() never waits forever
            }
        }

        m_engine.process(m_model_block.data(), m_model.m_input_size,
                         m_state.data(), m_model.m_state_size);

        for (size_t i = 0; i < m_model.m_input_size; ++i) {
            while (!m_output.try_push(m_model_block[i])) {
                if (!m_worker.running()) { return; }
            }
        }
    }

    OnnxEngine m_engine;               // Made with the processor, loaded in prepare()
    const ModelConfig m_model;         // Given at construction, never changes
    ThreadSafeRingBuffer m_input{0};   // Audio thread -> worker
    ThreadSafeRingBuffer m_output{0};  // Worker -> audio thread
    std::vector<float> m_model_block;  // The worker's
    std::vector<float> m_state;        // The worker's
    WorkerThread m_worker;
    size_t m_prefill = 0;            // Samples of silence m_output starts with: buffer adaptation + the worker's time
    size_t m_unpushed = 0;           // Samples that did not fit into m_input, owed as zeros
    size_t m_debt = 0;               // Samples that went out as zeros, thrown away when they arrive
    RingBuffer m_dry{0};             // The audio thread's own
    std::vector<float> m_dry_block;  // The audio thread's
    size_t m_latency = 0;
    float m_mix = 1.0f;
};

int main() {
    try {
        ProcessorExample processor(k_model);  // Keeps the model, loads nothing yet
        processor.prepare(k_host_block_size, k_max_inference_ms);  // Loads the model
        processor.set_mix(k_mix);

        // The host, as in step 6.
        std::vector<float> buffer(k_input_signal.begin(), k_input_signal.end());
        for (size_t start = 0; start < buffer.size(); start += k_host_block_size) {
            processor.process_block(buffer.data() + start, k_host_block_size);
            std::this_thread::sleep_for(block_duration(k_host_block_size, k_model.m_sample_rate));
        }

        processor.stop();

        // Dry and wet, both late by exactly the reported latency.
        return step_result(7, report_mix(buffer,
                                         k_input_signal,
                                         k_target_output_signal,
                                         k_mix,
                                         processor.latency_samples(),
                                         k_model.m_latency,
                                         k_host_block_size,
                                         k_model.m_sample_rate));
    } catch (const std::exception& error) {
        return step_failed(7, error);
    }
}
