// Step 9 — The model with anira
//
// Goal:   the part 1 model on anira: the model file and the contract say what
//         runs and how, the handler does steps 2, 6 and 7.
// Given:  contract.json, the fake host, anira.
// Task:   TODO 1 and 2 in model.json, TODO 3 below.
// Check:  the output equals the reference, shifted by get_latency().
//
// It builds and runs as it is, and says what is still missing.

#include <anira/anira.hpp>

#include <cstddef>
#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "common/helpers/host.h"
#include "common/helpers/support.h"
#include "common/assets/target_signal.h"
#include "common/assets/test_signal.h"

// The host side of the contract: what the fake host does, in samples at its rate.
constexpr struct {
    size_t m_block_size = 512;
    double m_sample_rate = 44100.0;
} k_host{};

// The model's own delay, as declared in model.json. The reference output
// already carries it, and so does the latency anira reports.
constexpr size_t k_model_latency = 2224;

namespace {

// The C handler behind a destructor, so an exception on the way does not leak it.
struct Handler {
    anira_handler* m_handle = nullptr;
    ~Handler() { anira_handler_destroy(m_handle); }
};

// The error message of a failed C call, with the status name in front.
std::string describe(const char* call, anira_status status, const anira_error& error) {
    return std::string(call) + ": " + anira_status_string(status) + ": " + error.message;
}

}  // namespace

int main() {
    const std::string step_dir = WORKSHOP_STEP_DIR;

    // The main thread plans: configs, context, handler, prepare.
    anira::Context context{anira::ContextConfig{}};
    Handler handler;
    size_t latency = 0;
    try {
        // ---- TODO 3 --------------------------------------------------------
        // Plan on the main thread:
        //   - load model.json and contract.json from step_dir
        //   - give the contract the host's geometry: block size twice, sample rate
        //   - build a pipeline with one inference stage over the model
        //   - anira_handler_create, then anira_handler_prepare with the contract
        //   - read the latency of output slot 0 into `latency`
        // Every C call returns a status and fills `error`; throw on anything but
        // ANIRA_OK, the catch below prints it.
        // --------------------------------------------------------------------
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }

    if (handler.m_handle == nullptr) {
        std::fprintf(stderr, "TODO 3: create and prepare the handler first.\n");
        return 2;
    }

    std::printf("Reported latency: %zu samples (%.1f ms at %.0f Hz)\n\n",
                latency,
                1000.0 * static_cast<double>(latency) / k_host.m_sample_rate,
                k_host.m_sample_rate);

    // One tensor over the host's block, built once: [channels, samples]. The
    // sample count is its shape, set per callback.
    int64_t shape[2] = {1, 0};
    anira_tensor block{};
    std::vector<size_t> late;  // Where a block that was not back in time went out as silence
    size_t position = 0;

    // The audio thread moves samples: in place, one call, never waits.
    const std::vector<float> output = run_host(
        k_input_signal.data(),
        k_input_signal.size(),
        k_host.m_block_size,
        [&](float* samples, size_t num_samples) {
            shape[1] = static_cast<int64_t>(num_samples);
            anira_tensor_init_host(&block, samples, ANIRA_DTYPE_F32, 2, shape);
            const anira_status status = anira_handler_process(handler.m_handle, &block, 0, &block, 0, nullptr);
            if (status == ANIRA_MISSED) { late.push_back(position); }  // The miss policy filled the block
            position += num_samples;
        },
        k_host.m_sample_rate);

    // The reference already carries the model's own delay, so it lines up with
    // the output after the rest of the reported latency, as in step 7.
    return report_with_latency(output.data(), k_target_output_signal.data(), output.size(),
                               latency - k_model_latency, late, k_host.m_block_size);
}
