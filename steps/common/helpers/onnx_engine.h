// The same model on ONNX Runtime, given complete.
//
// Goal: the same three methods as LibTorchEngine — construct, process, reset —
//       over an export that keeps its state in the open. The ONNX graph takes
//       audio plus 40 state tensors and returns audio plus 40 new ones, so the
//       state is carried here instead of inside the model.
#pragma once

#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
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

    // One block, processed in place.
    // @samples: the block to process, overwritten with the model's output
    // @num_samples: samples in that block
    void process(float* samples, size_t num_samples) {
        m_audio_shape[2] = static_cast<int64_t>(num_samples);
        m_inputs.push_back(Ort::Value::CreateTensor<float>(m_memory,
                                                           samples,
                                                           num_samples,
                                                           m_audio_shape.data(),
                                                           m_audio_shape.size()));

        for (size_t i = 0; i < m_state.size(); ++i) {
            m_inputs.push_back(Ort::Value::CreateTensor<float>(m_memory,
                                                               m_state[i].data(),
                                                               m_state[i].size(),
                                                               m_input_shapes[i + 1].data(),
                                                               m_input_shapes[i + 1].size()));
        }

        const auto outputs = m_session.Run(Ort::RunOptions{nullptr},
                                           m_input_name_pointers.data(),
                                           m_inputs.data(),
                                           m_inputs.size(),
                                           m_output_name_pointers.data(),
                                           m_output_name_pointers.size());

        const auto produced = outputs[0].GetTensorTypeAndShapeInfo().GetElementCount();
        if (produced != num_samples) {
            throw std::runtime_error("the model returned " + std::to_string(produced) +
                                     " samples for " + std::to_string(num_samples) + " in");
        }
        std::copy_n(outputs[0].GetTensorData<float>(), num_samples, samples);

        m_inputs.clear();  // Keeps the capacity the constructor reserved

        // Carry the new state over to the next call. Hidden in LibTorch, ours here.
        for (size_t i = 0; i < m_state.size(); ++i) {
            std::copy_n(outputs[i + 1].GetTensorData<float>(),
                        m_state[i].size(),
                        m_state[i].begin());
        }
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
