#include "engine/core/JobSystem.h"

#include "engine/core/Log.h"

#include <algorithm>
#include <chrono>

namespace br {

JobSystem::JobSystem(unsigned workerCount) {
    if (workerCount == 0) {
        const unsigned hw = std::max(2u, std::thread::hardware_concurrency());
        // Leave one core for the main/render thread.
        workerCount = std::max(1u, hw - 1u);
    }
    m_workers.reserve(workerCount);
    for (unsigned i = 0; i < workerCount; ++i) {
        m_workers.emplace_back([this] { workerLoop(); });
    }
    BR_LOG_INFO("jobs", "Job system started with {} worker threads", workerCount);
}

JobSystem::~JobSystem() {
    {
        std::lock_guard lock(m_mutex);
        m_stopping = true;
    }
    m_cv.notify_all();
    for (auto& t : m_workers) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void JobSystem::submit(std::function<void()> job, Priority priority) {
    {
        std::lock_guard lock(m_mutex);
        m_queues[static_cast<int>(priority)].push_back(std::move(job));
    }
    m_cv.notify_one();
}

usize JobSystem::pendingJobs() const {
    std::lock_guard lock(m_mutex);
    return m_queues[0].size() + m_queues[1].size() + m_queues[2].size();
}

bool JobSystem::tryRunOne() {
    std::function<void()> job;
    {
        std::lock_guard lock(m_mutex);
        for (auto& q : m_queues) {
            if (!q.empty()) {
                job = std::move(q.front());
                q.pop_front();
                break;
            }
        }
    }
    if (!job) {
        return false;
    }
    job();
    return true;
}

void JobSystem::workerLoop() {
    for (;;) {
        std::function<void()> job;
        {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [this] {
                return m_stopping || !m_queues[0].empty() || !m_queues[1].empty() || !m_queues[2].empty();
            });
            if (m_stopping && m_queues[0].empty() && m_queues[1].empty() && m_queues[2].empty()) {
                return;
            }
            for (auto& q : m_queues) {
                if (!q.empty()) {
                    job = std::move(q.front());
                    q.pop_front();
                    break;
                }
            }
        }
        if (job) {
            try {
                job();
            } catch (const std::exception& e) {
                // A job must never take the worker down; packaged_task jobs
                // forward exceptions through their future instead.
                BR_LOG_ERROR("jobs", "Unhandled exception in job: {}", e.what());
            }
        }
    }
}

void JobSystem::parallelFor(usize count, usize minBatch, const std::function<void(usize, usize)>& body) {
    if (count == 0) {
        return;
    }
    minBatch = std::max<usize>(1, minBatch);
    const usize maxBatches = static_cast<usize>(m_workers.size() + 1) * 4;
    const usize batches = std::clamp<usize>(count / minBatch, 1, maxBatches);
    if (batches <= 1) {
        body(0, count);
        return;
    }
    const usize per = (count + batches - 1) / batches;
    std::atomic<usize> remaining{0};
    std::mutex doneMutex;
    std::condition_variable doneCv;
    for (usize b = 1; b < batches; ++b) {
        const usize begin = b * per;
        if (begin >= count) {
            break;
        }
        const usize end = std::min(count, begin + per);
        remaining.fetch_add(1);
        submit(
            [&, begin, end] {
                body(begin, end);
                if (remaining.fetch_sub(1) == 1) {
                    std::lock_guard lock(doneMutex);
                    doneCv.notify_all();
                }
            },
            Priority::High);
    }
    body(0, std::min(count, per));
    // Help with queued work while waiting so nested parallelFor cannot deadlock.
    while (remaining.load() > 0) {
        if (!tryRunOne()) {
            std::unique_lock lock(doneMutex);
            doneCv.wait_for(lock, std::chrono::microseconds(200), [&] { return remaining.load() == 0; });
        }
    }
}

void MainThreadQueue::post(std::function<void()> fn) {
    std::lock_guard lock(m_mutex);
    m_items.push_back(std::move(fn));
}

usize MainThreadQueue::drain(double budgetMilliseconds) {
    const auto start = std::chrono::steady_clock::now();
    usize executed = 0;
    for (;;) {
        std::function<void()> fn;
        {
            std::lock_guard lock(m_mutex);
            if (m_items.empty()) {
                break;
            }
            fn = std::move(m_items.front());
            m_items.pop_front();
        }
        fn();
        ++executed;
        const double elapsed =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        if (elapsed >= budgetMilliseconds) {
            break;
        }
    }
    return executed;
}

usize MainThreadQueue::size() const {
    std::lock_guard lock(m_mutex);
    return m_items.size();
}

} // namespace br
