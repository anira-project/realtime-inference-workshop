// The same model on ONNX Runtime, given complete.
//
// Goal: an export that keeps its state in the open. The ONNX graph takes audio
//       plus the state (one tensor, 63488 floats: every convolution cache of
//       encoder and decoder) and returns audio plus the new state, so the caller
//       hands the state in and gets the new one back.
#pragma once

#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

class OnnxEngine {
public:
    // Nothing yet: no runtime, no model. load() does the work, so the engine can
    // be made long before the model is needed, as a plain member of a plugin.
    OnnxEngine() = default;

    // Loads the model, once: starts ONNX Runtime, reads the file and makes every
    // tensor. Takes a while and allocates, never on the audio thread.
    // @model_path: the .onnx file, graph and weights in one
    void load(const std::string& model_path) {
        if (loaded()) { throw std::runtime_error("the model is already loaded"); }
        m_env = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "workshop");

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

        // Input 0 and output 0 are the audio, input 1 and output 1 the state.
        // Every tensor is made once, here, with its own memory: process() only
        // fills and reads it. The outputs have the same shapes as the inputs.
        Ort::AllocatorWithDefaultOptions allocator;
        for (size_t i = 0; i < m_session.GetInputCount(); ++i) {
            m_input_names.emplace_back(m_session.GetInputNameAllocated(i, allocator).get());
            m_output_names.emplace_back(m_session.GetOutputNameAllocated(i, allocator).get());

            auto shape = m_session.GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
            for (auto& dimension : shape) {
                if (dimension < 0) { dimension = 1; }  // Symbolic batch size
            }
            m_inputs.push_back(Ort::Value::CreateTensor<float>(allocator, shape.data(), shape.size()));
            m_outputs.push_back(Ort::Value::CreateTensor<float>(allocator, shape.data(), shape.size()));
        }

        // Only now that the name vectors are final: a growing vector moves its
        // strings, and Run() wants plain pointers that stay valid.
        for (size_t i = 0; i < m_input_names.size(); ++i) {
            m_input_name_pointers.push_back(m_input_names[i].c_str());
            m_output_name_pointers.push_back(m_output_names[i].c_str());
        }

        m_num_samples = m_inputs[0].GetTensorTypeAndShapeInfo().GetElementCount();
        m_state_size = m_inputs[1].GetTensorTypeAndShapeInfo().GetElementCount();
    }

    bool loaded() const { return !m_inputs.empty(); }

    // One block without the state, processed in place. The model gets a zero
    // state on this call, as if the stream had just started.
    // @audio: the block, overwritten with the model's output
    // @num_samples: samples in the block; the model takes exactly one model block
    void process(float* audio, size_t num_samples) {
        check_loaded();
        std::fill_n(m_inputs[1].GetTensorMutableData<float>(), m_state_size, 0.0f);
        run(audio, num_samples);
    }

    // One block with the state, processed in place: the model continues where
    // the last call stopped.
    // @audio: the block, overwritten with the model's output
    // @num_samples: samples in the block; the model takes exactly one model block
    // @state: the model's state, zeros for a new stream; overwritten with the new state
    // @state_size: floats in the state; the model takes exactly its own state size
    void process(float* audio, size_t num_samples, float* state, size_t state_size) {
        check_loaded();
        if (state_size != m_state_size) {
            throw std::runtime_error("the state is exactly " + std::to_string(m_state_size) +
                                     " floats, got " + std::to_string(state_size));
        }
        std::copy_n(state, state_size, m_inputs[1].GetTensorMutableData<float>());
        run(audio, num_samples);
        std::copy_n(m_outputs[1].GetTensorMutableData<float>(), state_size, state);
    }

private:
    void check_loaded() const {
        if (!loaded()) { throw std::runtime_error("no model loaded: call load() first"); }
    }

    // The forward pass: the block in, through the model, the result back in place.
    void run(float* audio, size_t num_samples) {
        if (num_samples != m_num_samples) {
            throw std::runtime_error("the model takes exactly " + std::to_string(m_num_samples) +
                                     " samples per call, got " + std::to_string(num_samples));
        }
        std::copy_n(audio, num_samples, m_inputs[0].GetTensorMutableData<float>());

        m_session.Run(Ort::RunOptions{nullptr},
                      m_input_name_pointers.data(),
                      m_inputs.data(),
                      m_inputs.size(),
                      m_output_name_pointers.data(),
                      m_outputs.data(),
                      m_outputs.size());

        std::copy_n(m_outputs[0].GetTensorMutableData<float>(), num_samples, audio);
    }

    Ort::Env m_env{nullptr};
    Ort::Session m_session{nullptr};

    std::vector<std::string> m_input_names;
    std::vector<std::string> m_output_names;
    std::vector<const char*> m_input_name_pointers;
    std::vector<const char*> m_output_name_pointers;
    std::vector<Ort::Value> m_inputs;   // audio, state
    std::vector<Ort::Value> m_outputs;  // audio, state
    size_t m_num_samples = 0;
    size_t m_state_size = 0;
};
