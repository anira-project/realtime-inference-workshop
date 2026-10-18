#pragma once

#include <anira/anira.h>
#include <array>
#include <cstddef>
#include <utility>

// One anira session in realtime streaming mode: one pop scheduled per push, priming zeros skipped, push-time contexts paired FIFO with popped results.
// Never blocks on inference; no locks and no heap allocation once constructed.
namespace anira::models::handtracking {

template <typename Context, size_t Capacity>
class StreamedSession {
public:
    // anira requires the wait strategy to match across every CoreConfig in the process — pass the same config to every session.
    StreamedSession(anira::InferenceConfig config, const anira::CoreConfig& context_config)
        : m_config(std::move(config)), m_pp(m_config), m_handler(m_pp, m_config, context_config) {}

    anira::InferenceHandler& handler() { return m_handler; }
    anira::PrePostProcessor& pp() { return m_pp; }  // non-streamable tensors

    // deadline_ms must cover thread-pool queueing behind other models, not just execution time: too small -> silently dropped pushes, permanently desynced session.
    // pushes_per_second is the worst-case push rate; anira sizes its buffers from it.
    void prepare(anira::InferenceBackend backend,
                 float deadline_ms,
                 int values_per_push,
                 size_t out0_size,
                 double pushes_per_second) {
        m_config.m_max_inference_time = deadline_ms;
        m_handler.set_inference_backend(backend);
        m_handler.prepare(anira::HostConfig(
            static_cast<float>(values_per_push),
            static_cast<float>(values_per_push) * static_cast<float>(pushes_per_second)));
        m_primed_skips = m_handler.get_latency(0) / out0_size;
        m_pending_pops = 0;
        m_in_flight.clear();
    }

    // Not realtime-safe: reset() waits for pending inferences.
    void switchBackend(anira::InferenceBackend backend,
                       float deadline_ms,
                       int values_per_push,
                       size_t out0_size,
                       double pushes_per_second) {
        m_handler.reset();
        prepare(backend, deadline_ms, values_per_push, out0_size, pushes_per_second);
    }

    bool canPush() const { return m_in_flight.count < Capacity; }

    // Call right after handler().push_data(): queues the context and schedules the matching pop.
    void recordPush(const Context& context) {
        m_in_flight.push(context);
        ++m_pending_pops;
    }

    // try_pop returns false on underrun (the pop stays scheduled, retried next frame); handle_result gets the paired context for every real result.
    template <typename TryPopFn, typename HandleResultFn>
    void drainResults(TryPopFn&& try_pop, HandleResultFn&& handle_result) {
        while (m_pending_pops > 0) {
            if (!try_pop())
                return;
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

    // Turns leftover in-flight results into skips so future pushes pair with fresh results. Only call when allPopsServed().
    void discardInFlightResults() {
        m_primed_skips += m_in_flight.count;
        m_in_flight.clear();
    }

private:
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
            while (count > 0)
                pop();
        }
    };

    anira::InferenceConfig m_config;
    anira::PrePostProcessor m_pp;
    anira::InferenceHandler m_handler;

    ContextQueue m_in_flight;
    size_t m_pending_pops = 0;
    size_t m_primed_skips = 0;
};

}  // namespace anira::models::handtracking
