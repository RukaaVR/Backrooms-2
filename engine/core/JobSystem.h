#pragma once

#include "engine/core/Types.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace br {

// General-purpose worker pool for CPU work that must not stall the frame:
// chunk generation, procedural texture synthesis, sound synthesis, navigation
// baking. GPU/OpenGL work is never executed here; workers hand results back to
// the main thread through MainThreadQueue.
class JobSystem {
public:
    enum class Priority { High, Normal, Low };

    explicit JobSystem(unsigned workerCount = 0);
    ~JobSystem();
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    void submit(std::function<void()> job, Priority priority = Priority::Normal);

    template <typename F>
    auto async(F&& fn, Priority priority = Priority::Normal) -> std::future<std::invoke_result_t<F>> {
        using R = std::invoke_result_t<F>;
        auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(fn));
        std::future<R> future = task->get_future();
        submit([task]() { (*task)(); }, priority);
        return future;
    }

    // Splits [0, count) into contiguous ranges and blocks until all are done.
    // The calling thread participates so this is safe to call from a worker.
    void parallelFor(usize count, usize minBatch, const std::function<void(usize begin, usize end)>& body);

    unsigned workerCount() const { return static_cast<unsigned>(m_workers.size()); }
    usize pendingJobs() const;

private:
    void workerLoop();
    bool tryRunOne();

    std::vector<std::thread> m_workers;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<std::function<void()>> m_queues[3];
    bool m_stopping = false;
};

// Multi-producer queue drained once per frame on the thread that owns the GL
// context. Used for GPU uploads and anything else that must happen there.
class MainThreadQueue {
public:
    void post(std::function<void()> fn);
    // Runs queued work until the queue is empty or the time budget is exhausted.
    // Returns the number of items executed.
    usize drain(double budgetMilliseconds = 1e9);
    usize size() const;

private:
    mutable std::mutex m_mutex;
    std::deque<std::function<void()>> m_items;
};

} // namespace br
