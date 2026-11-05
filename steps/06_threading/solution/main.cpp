// Step 6 — Inference on a worker thread, reference implementation
//
// Goal:   get the engine off the audio thread and keep the latency fixed.
// Given:  ThreadSafeRingBuffer, step 3's ring buffer made safe between two
//         threads, and WorkerThread.
// Task:   fill in ProcessorExample, at the three TODO banners.
// Check:  the host gets the model's stream back, exactly the prefill later.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <thread>
#include <vector>

#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"
#include "common/checks/step06.h"
#include "common/helpers/host.h"
#include "common/helpers/onnx_engine.h"
#include "common/helpers/threadsafe_ring_buffer.h"
#include "common/helpers/worker_thread.h"
#include "common/inference_time.h"

// Model settings taken from the export metadata, as in step 1.
constexpr struct {
    const char* m_path = WORKSHOP_ONNX_MODEL_PATH;
    size_t m_input_size = 2048;
    size_t m_state_size = 63488;
    double m_sample_rate = 44100.0;
} k_model{};

// What the host hands over per callback: four callbacks make one model block.
constexpr size_t k_host_block_size = 512;

// The first calls are slow (step 4): made before the audio starts.
constexpr size_t k_warm_up_calls = 2;

// Step 3's processor, split over two threads. The audio thread only moves
// samples, the worker runs the model:
//
//   audio thread:   host block -> m_input              m_output -> host block
//   worker thread:                m_input -> model -> m_output
//
// m_output starts with the prefill: silence, so the worker has time and the
// host always finds something to take.
class ProcessorExample {
public:
    explicit ProcessorExample(OnnxEngine& engine) : m_engine(engine) {}

    void prepare(size_t max_block_size) {
        m_worker.stop();

        // ---- TODO 1 --------------------------------------------------------
        // How late does the host hear the model?
        //   1. rounded_max_inference_time: k_max_inference_ms in samples,
        //      rounded up to whole callbacks of max_block_size samples
        //   2. the prefill: the buffer adaptation from step 3 plus the time
        //      the worker is given
        //   3. the size of m_output: the worker's time plus one model block
        // --------------------------------------------------------------------
        const double max_inference_samples = k_max_inference_ms * k_model.m_sample_rate / 1000.0;
        const size_t rounded_max_inference_time =
            static_cast<size_t>(
                std::ceil(max_inference_samples / static_cast<double>(max_block_size))) *
            max_block_size;  // <- solution
        m_prefill =
            (k_model.m_input_size - max_block_size) + rounded_max_inference_time;  // <- solution
        const size_t output_capacity =
            rounded_max_inference_time + k_model.m_input_size;  // <- solution

        check_todos(k_max_inference_ms,
                    rounded_max_inference_time,
                    m_prefill,
                    output_capacity,
                    max_block_size,
                    k_model.m_sample_rate);  // Given: names a missing TODO

        m_input = ThreadSafeRingBuffer(k_model.m_input_size);
        m_output = ThreadSafeRingBuffer(output_capacity);
        const std::vector<float> silence(m_prefill, 0.0f);
        m_output.push(silence.data(), m_prefill);  // The prefill
        m_model_block.assign(k_model.m_input_size, 0.0f);
        m_state.assign(k_model.m_state_size, 0.0f);

        // The warm-up: the first calls are slow (step 4), so they are made
        // here, before the audio starts. Then the state starts at zero again.
        for (size_t i = 0; i < k_warm_up_calls; ++i) {
            m_engine.process(m_model_block.data(), k_model.m_input_size, m_state);
        }
        std::fill(m_state.begin(), m_state.end(), 0.0f);
        m_unpushed = 0;
        m_debt = 0;

        // From here on, process_model() runs over and over on the worker thread.
        m_worker.start([this] { process_model(); });
    }

    void stop() { m_worker.stop(); }

    // The audio thread: one callback. No engine anymore, only samples in and out.
    void process_block(float* samples, size_t num_samples) {
        // ---- TODO 2 --------------------------------------------------------
        // Step 3's process_block(), without the model, one sample at a time:
        //   1. push the host's samples into m_input, one by one. If one does
        //      not fit, the worker is a whole block behind: count it and the
        //      rest in m_unpushed, and stop
        //   2. (given) those never reach the model: the host gets as many
        //      zeros instead, and nothing is taken out of m_output for them
        //   3. take the rest from m_output, one by one. If one is not there
        //      yet, or something is still owed, the worker is late: it and the
        //      rest of this callback go out as zeros, and all of them are
        //      owed: m_debt
        //   4. pay the debt first: what is owed came too late, its place
        //      already went out as a zero. Take it out and throw it away
        //
        // Hint: try_push() and try_pop() never wait, they say whether it worked.
        // --------------------------------------------------------------------
        for (size_t i = 0; i < num_samples; ++i) {
            const bool pushed = m_input.try_push(samples[i]);  // <- solution
            if (!pushed) {
                m_unpushed += num_samples - i;  // <- solution
                break;
            }
        }

        const size_t zeros = std::min(m_unpushed, num_samples);
        std::fill_n(samples, zeros, 0.0f);
        m_unpushed -= zeros;

        float late = 0.0f;
        while (m_debt > 0) {
            if (m_output.try_pop(late)) {  // <- solution
                --m_debt;
            } else {
                break;
            }
        }

        for (size_t i = zeros; i < num_samples; ++i) {
            const bool popped = m_debt == 0 && m_output.try_pop(samples[i]);  // <- solution
            if (!popped) {
                // Not the next one either: while anything is owed, or while
                // the worker is pushing right now, what comes next would land
                // one place too early.
                std::fill_n(samples + i, num_samples - i, 0.0f);
                m_debt += num_samples - i;  // <- solution
                break;
            }
        }
    }

    // How late the host hears the model's stream, in samples.
    size_t prefill_samples() const { return m_prefill; }

private:
    // The worker thread: called over and over. The engine may allocate and
    // lock here, nobody is waiting on this thread.
    void process_model() {
        // ---- TODO 3 --------------------------------------------------------
        // The model's half of step 3's process_block():
        //   1. check if m_input has enough samples for a model block
        //   2. if so: pop it into m_model_block, run the model (given), push
        //      the result into m_output
        // --------------------------------------------------------------------
        const bool has_block = m_input.available() >= k_model.m_input_size;  // <- solution

        if (has_block) {
            m_input.pop(m_model_block.data(), k_model.m_input_size);  // <- solution
            m_engine.process(m_model_block.data(), k_model.m_input_size, m_state);
            m_output.push(m_model_block.data(), k_model.m_input_size);  // <- solution
        }
    }

    OnnxEngine& m_engine;
    ThreadSafeRingBuffer m_input{0};   // Audio thread -> worker
    ThreadSafeRingBuffer m_output{0};  // Worker -> audio thread
    std::vector<float> m_model_block;  // The worker's
    std::vector<float> m_state;        // The worker's
    WorkerThread m_worker;
    size_t m_prefill = 0;
    size_t m_unpushed = 0;  // Samples that did not fit into m_input, owed as zeros
    size_t m_debt = 0;      // Samples that went out as zeros, thrown away when they arrive
};

int main() {
    try {
        OnnxEngine engine(k_model.m_path);

        ProcessorExample processor(engine);
        processor.prepare(k_host_block_size);

        // The host, as in step 3, with one difference: a sleep after every
        // callback. Two threads only work if the audio thread is called at the
        // audio rate, one block every 11.6 ms, not as fast as the loop can go.
        // The sleep simulates the audio device.
        std::vector<float> output(k_input_signal.begin(), k_input_signal.end());
        for (size_t start = 0; start < output.size(); start += k_host_block_size) {
            processor.process_block(output.data() + start, k_host_block_size);
            std::this_thread::sleep_for(block_duration(k_host_block_size, k_model.m_sample_rate));
        }

        processor.stop();

        // Silence for the prefill, then the model's stream.
        return report_with_prefill(output,
                                   k_target_output_signal,
                                   processor.prefill_samples(),
                                   k_host_block_size,
                                   k_model.m_sample_rate);
    } catch (const std::exception& error) {
        std::printf("FAILED: %s\n", error.what());
        return 1;
    }
}
