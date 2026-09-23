// The inference engine for step 4, given complete.
//
// Goal: LibTorch behind three methods — construct, reset, process. Later steps
//       keep this interface and change who calls process(), and from which
//       thread.
#pragma once

#include <torch/script.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace workshop {

class LibTorchEngine {
public:
    // Loads the TorchScript file: graph and weights in one file, so there is
    // no model class to link against. Throws if the file cannot be read.
    explicit LibTorchEngine(const std::string& model_path) {
        try {
            m_model = torch::jit::load(model_path);
        } catch (const c10::Error& error) {
            throw std::runtime_error("cannot load " + model_path + "\n" + error.what());
        }
        m_model.eval();  // Inference mode: no dropout, no batchnorm updates
    }

    // Clears the model's state, for a new stream — what prepare() does in a
    // plugin. A freshly loaded model is already cleared.
    void reset() { m_model.run_method("reset_state"); }

    // One block, processed in place — the shape an audio callback hands you.
    // The model keeps its state internally, so consecutive calls continue where
    // the last one ended.
    // @samples: the block to process, overwritten with the model's output
    // @num_samples: samples in that block
    void process(float* samples, size_t num_samples) {
        const torch::NoGradGuard no_grad;  // No autograd graph, no extra allocations

        // A tensor around the caller's samples: {batch, channels, samples}.
        // from_blob does not copy, so `samples` has to outlive `input`.
        const auto length = static_cast<int64_t>(num_samples);
        const auto input = torch::from_blob(samples, {1, 1, length}, torch::kFloat32);

        const auto output = m_model.forward({input}).toTensor().contiguous();

        // The model emits whole blocks: a length it was not exported for comes
        // back a different size, which would leave the buffer half stale.
        if (output.numel() != length) {
            throw std::runtime_error("the model returned " + std::to_string(output.numel()) +
                                     " samples for " + std::to_string(num_samples) + " in");
        }

        std::copy_n(output.data_ptr<float>(), num_samples, samples);
    }

private:
    torch::jit::script::Module m_model;
};

}  // namespace workshop
