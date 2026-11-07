#pragma once

#include <anira/anira.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

// One anira handler in realtime streaming mode: one pop scheduled per push, priming zeros skipped,
// push-time contexts paired FIFO with popped results.
// Never blocks on inference; no locks and no heap allocation once prepared.
namespace anira::models::handtracking {

// `count` floats as one Streamed block: one channel of `count` values.
inline anira::Tensor host_block(float* data, std::size_t count) {
    const int64_t shape[2] = {1, static_cast<int64_t>(count)};
    return anira::Tensor::from_host(data, ANIRA_DTYPE_F32, shape);
}

// One float as a Static value, shape {1, 1}.
inline anira::Tensor host_scalar(float* data) {
    const int64_t shape[2] = {1, 1};
    return anira::Tensor::from_host(data, ANIRA_DTYPE_F32, shape);
}

// Whether a whole result is waiting in output 0: a pop with nothing waiting is a miss.
inline bool has_result(anira::InferenceHandler& handler, std::size_t num_values) {
    std::size_t available = 0;
    handler.get_available_samples(0, 0, &available);
    return available >= num_values;
}

template <typename Context, size_t Capacity>
class StreamedSession {
public:
    // Every session of the tracker shares one anira::Context.
    StreamedSession(anira::Context& context, const anira::ModelConfig& model)
        : m_handler(make_handler(context, model)) {}

    anira::InferenceHandler& handler() { return m_handler; }

    // deadline_ms must cover thread-pool queueing behind other models, not just execution time:
    // too small -> silently dropped pushes, permanently desynced session.
    // pushes_per_second is the worst-case push rate; anira sizes its buffers from it.
    void prepare(float deadline_ms,
                 int values_per_push,
                 size_t out0_size,
                 double pushes_per_second) {
        anira::Hard hard;
        // Each push is one whole input: the host block is exactly one window
        hard.host_block_min = values_per_push;
        hard.host_block_max = values_per_push;
        hard.host_rate = static_cast<double>(values_per_push) * pushes_per_second;
        hard.budget = std::chrono::duration<float, std::milli>(deadline_ms);
        hard.warmup = ANIRA_WARMUP_FIXED;
        hard.warmup_iterations = 1;
        hard.on_miss = ANIRA_MISS_ZEROS;
        m_handler.prepare(hard);  // Throws with anira's reason

        m_primed_skips = m_handler.get_latency(0) / out0_size;
        m_pending_pops = 0;
        m_in_flight.clear();
    }

    bool canPush() const { return m_in_flight.count < Capacity; }

    // Call right after handler().push_data(): queues the context and schedules the matching pop.
    void recordPush(const Context& context) {
        m_in_flight.push(context);
        ++m_pending_pops;
    }

    // try_pop returns false while the result is not there yet (the pop stays scheduled, retried
    // next frame); handle_result gets the paired context for every real result.
    template <typename TryPopFn, typename HandleResultFn>
    void drainResults(TryPopFn&& try_pop, HandleResultFn&& handle_result) {
        while (m_pending_pops > 0) {
            if (!try_pop()) { return; }
            --m_pending_pops;
            if (m_primed_skips > 0) {
                --m_primed_skips;
                continue;  // latency-priming zeros, not a real result
            }
            const Context context = m_in_flight.front();
            m_in_flight.pop();
            handle_result(context);
        }
    }

    bool allPopsServed() const { return m_pending_pops == 0; }
    bool hasInFlightResults() const { return m_in_flight.count > 0; }

    // Turns leftover in-flight results into skips so future pushes pair with fresh results. Only
    // call when allPopsServed().
    void discardInFlightResults() {
        m_primed_skips += m_in_flight.count;
        m_in_flight.clear();
    }

private:
    static anira::InferenceHandler make_handler(anira::Context& context,
                                                const anira::ModelConfig& model) {
        anira::Pipeline pipeline;
        pipeline.add_inference(model);
        return anira::InferenceHandler(context, pipeline);
    }

    // Fixed-capacity FIFO — no heap allocation on the video thread.
    struct ContextQueue {
        std::array<Context, Capacity> items{};
        size_t head = 0;
        size_t count = 0;

        Context& front() { return items[head]; }
        void pop() {
            items[head] = Context{};  // release refcounted members (Image)
            head = (head + 1) % Capacity;
            --count;
        }
        void push(const Context& value) {
            items[(head + count) % Capacity] = value;
            ++count;
        }
        void clear() {
            while (count > 0) { pop(); }
        }
    };

    anira::InferenceHandler m_handler;

    ContextQueue m_in_flight;
    size_t m_pending_pops = 0;
    size_t m_primed_skips = 0;
};

}  // namespace anira::models::handtracking
