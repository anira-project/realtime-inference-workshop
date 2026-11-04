// The same model on ONNX Runtime, given complete.
//
// Goal: the same three methods as LibTorchEngine — construct, process, reset —
//       over an export that keeps its state in the open. The ONNX graph takes
//       audio plus the state (one tensor, 63488 floats: every convolution
//       cache of encoder and decoder) and returns audio plus the new state, so
//       the state is carried here instead of inside the model.
#pragma once

#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

class OnnxEngine {
public:
    // @model_path: the .onnx file; its weights sit next to it in a .data file
    explicit OnnxEngine(const std::string& model_path)
        : m_env(ORT_LOGGING_LEVEL_WARNING, "workshop")
        , m_memory(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)) {
        Ort::SessionOptions options;
        options.SetIntraOpNumThreads(1);  // One thread, so the timing is the model's

        try {
#ifdef _WIN32
            const std::wstring wide(model_path.begin(), model_path.end());
            m_session = Ort::Session(m_env, wide.c_str(), options);
#else
            m_session = Ort::Session(m_env, model_path.c_str(), options);
#endif
        } catch (const Ort::Exception& error) {
            throw std::runtime_error("cannot load " + model_path + "\n" + error.what());
        }

        // Input 0 and output 0 are the audio; everything after them is state.
        Ort::AllocatorWithDefaultOptions allocator;
        m_input_names.reserve(m_session.GetInputCount());
        m_output_names.reserve(m_session.GetOutputCount());

        for (size_t i = 0; i < m_session.GetInputCount(); ++i) {
            m_input_names.emplace_back(m_session.GetInputNameAllocated(i, allocator).get());

            auto shape = m_session.GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
            for (auto& dimension : shape) {
                if (dimension < 0) { dimension = 1; }  // Symbolic batch size
            }
            m_input_shapes.push_back(shape);
        }
        for (size_t i = 0; i < m_session.GetOutputCount(); ++i) {
            m_output_names.emplace_back(m_session.GetOutputNameAllocated(i, allocator).get());
        }

        // Only now that the name vectors are final: a growing vector moves its
        // strings, and Run() wants plain pointers that stay valid.
        for (const std::string& name : m_input_names) {
            m_input_name_pointers.push_back(name.c_str());
        }
        for (const std::string& name : m_output_names) {
            m_output_name_pointers.push_back(name.c_str());
        }

        m_state.resize(m_input_shapes.size() - 1);
        m_inputs.reserve(m_input_shapes.size());  // So process() allocates nothing of its own
        reset();
    }

    // Clears the state, for a new stream — what prepare() does in a plugin.
    void reset() {
        for (size_t i = 0; i < m_state.size(); ++i) {
            m_state[i].assign(elements(m_input_shapes[i + 1]), 0.0f);
        }
    }

    // One block, with everything the model takes passed in, processed in place.
    // @data: {audio} or {audio, state}: your vectors themselves, not copies of them.
    //        Without the state the model gets a zero state on this call, as if the
    //        stream had just started. The block in audio is overwritten with the
    //        model's output, and state with the new state.
    // @sizes: sizes[0] is the samples in this block, sizes[1] the size of the state
    // @start: where the block starts in data[0]
    void process(const std::vector<std::reference_wrapper<std::vector<float>>>& data,
                 const std::vector<size_t>& sizes,
                 size_t start = 0) {
        if (data.empty() || data.size() > 2 || data.size() != sizes.size()) {
            throw std::runtime_error("got " + std::to_string(data.size()) + " buffers but " +
                                     std::to_string(sizes.size()) +
                                     " sizes: pass the audio, and optionally the state, with one size per buffer");
        }
        const size_t num_samples = sizes[0];
        if (num_samples != elements(m_input_shapes[0])) {
            throw std::runtime_error("the model takes exactly " + std::to_string(elements(m_input_shapes[0])) +
                                     " samples per call, got " + std::to_string(num_samples));
        }
        if (start + num_samples > data[0].get().size()) {
            throw std::runtime_error("the block at sample " + std::to_string(start) +
                                     " runs past the end of the audio");
        }
        const bool has_state = data.size() == 2;
        if (has_state && (sizes[1] != m_state[0].size() || data[1].get().size() != m_state[0].size())) {
            throw std::runtime_error("the state is exactly " + std::to_string(m_state[0].size()) + " floats, got size " +
                                     std::to_string(sizes[1]) + " and a buffer of " + std::to_string(data[1].get().size()));
        }

        float* samples = data[0].get().data() + start;
        std::vector<float>& state = has_state ? data[1].get() : m_state[0];  // m_state stays all zeros

        m_audio_shape[2] = static_cast<int64_t>(num_samples);
        m_inputs.push_back(Ort::Value::CreateTensor<float>(m_memory,
                                                           samples,
                                                           num_samples,
                                                           m_audio_shape.data(),
                                                           m_audio_shape.size()));
        m_inputs.push_back(Ort::Value::CreateTensor<float>(m_memory,
                                                           state.data(),
                                                           state.size(),
                                                           m_input_shapes[1].data(),
                                                           m_input_shapes[1].size()));

        const auto outputs = m_session.Run(Ort::RunOptions{nullptr},
                                           m_input_name_pointers.data(),
                                           m_inputs.data(),
                                           m_inputs.size(),
                                           m_output_name_pointers.data(),
                                           m_output_name_pointers.size());

        std::copy_n(outputs[0].GetTensorData<float>(), num_samples, samples);

        m_inputs.clear();  // Keeps the capacity the constructor reserved

        // The new state goes back to the caller, for the next call.
        if (has_state) { std::copy_n(outputs[1].GetTensorData<float>(), state.size(), state.begin()); }
    }

private:
    static size_t elements(const std::vector<int64_t>& shape) {
        size_t count = 1;
        for (const int64_t dimension : shape) { count *= static_cast<size_t>(dimension); }
        return count;
    }

    Ort::Env m_env;
    Ort::MemoryInfo m_memory;
    Ort::Session m_session{nullptr};
    std::vector<std::string> m_input_names;
    std::vector<std::string> m_output_names;
    std::vector<const char*> m_input_name_pointers;
    std::vector<const char*> m_output_name_pointers;
    std::vector<std::vector<int64_t>> m_input_shapes;
    std::vector<std::vector<float>> m_state;
    std::vector<Ort::Value> m_inputs;
    std::vector<int64_t> m_audio_shape{1, 1, 0};
};
