// Step 6 — Inference on a worker thread, reference implementation
//
// Goal:   get the engine off the audio thread and keep the latency fixed.
// Given:  ThreadSafeRingBuffer, step 3's ring buffer made safe between two
//         threads, and WorkerThread.
// Task:   fill in ProcessorExample, at the three TODO banners.
// Check:  the host gets the model's stream back, exactly the prefill later.

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

        // Given: the worker's time, your worst case from step 4 (k_max_inference_ms)
        // in samples, rounded up to whole callbacks: the audio thread only picks
        // results up once per callback.
        const double worst_case_samples = k_max_inference_ms * k_model.m_sample_rate / 1000.0;
        const size_t worker_time =
            static_cast<size_t>(std::ceil(worst_case_samples / max_block_size)) * max_block_size;

        // ---- TODO 1 --------------------------------------------------------
        // The prefill: how much silence m_output starts with, so the host
        // always finds something to take. Step 3's prefill, and now the worker
        // needs time on top to run the model block.
        //
        // Hint: the worker's time is worker_time, just above.
        // --------------------------------------------------------------------
        m_prefill = (k_model.m_input_size - max_block_size) + worker_time;  // <- solution

        check_prefill(m_prefill,
                      max_block_size,
                      k_model.m_input_size,
                      worker_time);  // Given: names TODO 1 if it is off

        // Given: m_input holds one model block; m_output the worker's time and
        // one model block more, so the prefill fits
        m_input = ThreadSafeRingBuffer(k_model.m_input_size);
        m_output = ThreadSafeRingBuffer(worker_time + k_model.m_input_size);
        for (size_t i = 0; i < m_prefill; ++i) {
            const bool pushed = m_output.try_push(0.0f);
            assert(pushed);  // never fails: m_output has room for the prefill
        }
        m_model_block.assign(k_model.m_input_size, 0.0f);
        m_state.assign(k_model.m_state_size, 0.0f);

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
        // The audio thread.
        //   1. push the host's samples into m_input, one by one. Use try_push(),
        //      to check if they fit, so the audio thread never blocks.
        //   2. collect completed blocks from the worker: pop them from m_output,
        //      one by one. Use try_pop() to check if they are there, so the audio
        //      thread never blocks.
        //
        // Hint: try_push() and try_pop() never wait, they say whether it worked.
        // --------------------------------------------------------------------
        for (size_t i = 0; i < num_samples; ++i) {
            const bool pushed = m_input.try_push(samples[i]);  // <- solution
            if (!pushed) {
                m_unpushed += num_samples - i;
                break;
            }
        }

        // The zeros owed for m_unpushed go out first. A callback can hold at most
        // num_samples of them; whatever is left waits for the next callback.
        const size_t zeros = std::min(m_unpushed, num_samples);
        std::fill_n(samples, zeros, 0.0f);
        m_unpushed -= zeros;

        // Pay the debt first: these samples came too late, their place already went
        // out as zeros. Throw them away, so the stream stays in its place.
        float late = 0.0f;
        while (m_debt > 0) {
            if (m_output.try_pop(late)) {
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
                m_debt += num_samples - i;
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
        // The worker, sample by sample like the audio thread, but this thread
        // may wait: when a try_ fails, it tries again right away, a hot spin.
        //   1. pop a whole model block from m_input into m_model_block
        //   2. push the result into m_output
        // The inference in between is step 3's, given.
        // --------------------------------------------------------------------
        for (size_t i = 0; i < k_model.m_input_size; ++i) {
            while (!m_input.try_pop(m_model_block[i])) {  // <- solution
                if (!m_worker.running()) { return; }  // given: stop() never waits forever
            }
        }

        m_engine.process(m_model_block.data(), k_model.m_input_size, m_state.data(), m_state.size());

        for (size_t i = 0; i < k_model.m_input_size; ++i) {
            while (!m_output.try_push(m_model_block[i])) {  // <- solution
                if (!m_worker.running()) { return; }  // given
            }
        }
    }

    OnnxEngine& m_engine;
    ThreadSafeRingBuffer m_input{0};   // Audio thread -> worker
    ThreadSafeRingBuffer m_output{0};  // Worker -> audio thread
    std::vector<float> m_model_block;  // The worker's
    std::vector<float> m_state;        // The worker's
    WorkerThread m_worker;
    // The silence m_output starts with, in samples: how late the host hears the stream.
    size_t m_prefill = 0;
    // Samples that did not fit into m_input, because the worker is a whole block
    // behind. They never reach the model, so nothing comes out of m_output for them:
    // the host gets a zero for each one instead, and nothing is popped.
    size_t m_unpushed = 0;
    // Samples the host needed before m_output had them, so they went out as zeros. The
    // model does still deliver them, too late: they are thrown away when they arrive.
    size_t m_debt = 0;
};

int main() {
    try {
        OnnxEngine engine;
        engine.load(k_model.m_path);

        ProcessorExample processor(engine);
        processor.prepare(k_host_block_size);

        // The host, as in step 3, with one difference: a sleep after every
        // callback. Two threads only work if the audio thread is called at the
        // audio rate, one block every 11.6 ms, not as fast as the loop can go.
        // The sleep simulates the audio device.
        std::vector<float> buffer(k_input_signal.begin(), k_input_signal.end());
        for (size_t start = 0; start < buffer.size(); start += k_host_block_size) {
            processor.process_block(buffer.data() + start, k_host_block_size);
            std::this_thread::sleep_for(block_duration(k_host_block_size, k_model.m_sample_rate));
        }

        processor.stop();

        // Silence for the prefill, then the model's stream.
        return report_with_prefill(buffer,
                                   k_target_output_signal,
                                   processor.prefill_samples(),
                                   k_host_block_size,
                                   k_model.m_sample_rate);
    } catch (const std::exception& error) {
        std::printf("FAILED: %s\n", error.what());
        return 1;
    }
}
