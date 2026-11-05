// A worker thread, given complete.
//
// Goal: run one function over and over on its own thread, next to the audio
//       thread (step 6), so that the exercise only has to say what it does.
#pragma once

#include <atomic>
#include <functional>
#include <thread>
#include <utility>

class WorkerThread {
public:
    WorkerThread() = default;
    ~WorkerThread() { stop(); }
    WorkerThread(const WorkerThread&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;

    // Starts the thread: it calls `work` over and over until stop(). No sleep
    // in between, the result is needed soon. Allocates: call it in prepare().
    void start(std::function<void()> work) {
        stop();
        m_running.store(true, std::memory_order_release);
        m_thread = std::thread([this, work = std::move(work)] {
            while (m_running.load(std::memory_order_acquire)) { work(); }
        });
    }

    // Waits for the current call of `work` to finish, then ends the thread.
    void stop() {
        if (!m_thread.joinable()) { return; }
        m_running.store(false, std::memory_order_release);
        m_thread.join();
    }

private:
    std::thread m_thread;
    std::atomic<bool> m_running{false};
};
