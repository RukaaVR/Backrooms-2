#pragma once

#include "engine/core/Types.h"

#include <atomic>
#include <chrono>
#include <string_view>
#include <vector>

// CPU profiling hooks.
//
// BR_PROFILE_SCOPE("name") instruments a scope. With -DBR_ENABLE_TRACY=ON the
// macros forward to the Tracy profiler (live remote capture). Otherwise a
// built-in, lock-free aggregator records per-call-site totals that the game
// reads once per frame for the debug overlay and the performance report.
//
// Built-in design: every call site owns one static ScopeStat registered on
// first use; scope exit performs two relaxed atomic adds. No allocation, no
// locks in the hot path, safe from worker threads.

#if defined(BR_ENABLE_TRACY) && BR_ENABLE_TRACY
#include <tracy/Tracy.hpp>
#define BR_PROFILE_SCOPE(name) ZoneScopedN(name)
#define BR_PROFILE_FRAME() FrameMark
#define BR_PROFILE_THREAD(name) tracy::SetThreadName(name)
#else
#define BR_PROFILE_CONCAT_INNER(a, b) a##b
#define BR_PROFILE_CONCAT(a, b) BR_PROFILE_CONCAT_INNER(a, b)
#define BR_PROFILE_SCOPE(name)                                                                          \
    static ::br::profile::ScopeStat BR_PROFILE_CONCAT(brProfStat_, __LINE__){name};                     \
    const ::br::profile::ScopedTimer BR_PROFILE_CONCAT(brProfTimer_, __LINE__){BR_PROFILE_CONCAT(brProfStat_, __LINE__)}
#define BR_PROFILE_FRAME() ::br::profile::endFrame()
#define BR_PROFILE_THREAD(name) ((void)(name))
#endif

namespace br::profile {

class ScopeStat {
public:
    explicit ScopeStat(std::string_view name);
    ScopeStat(const ScopeStat&) = delete;
    ScopeStat& operator=(const ScopeStat&) = delete;

    void add(u64 nanoseconds) {
        m_ns.fetch_add(nanoseconds, std::memory_order_relaxed);
        m_calls.fetch_add(1, std::memory_order_relaxed);
    }
    std::string_view name() const { return m_name; }

private:
    friend void endFrame();
    friend struct Registry;
    std::string_view m_name;
    std::atomic<u64> m_ns{0};
    std::atomic<u32> m_calls{0};
    double m_lastMs = 0.0;
    double m_avgMs = 0.0;
    u32 m_lastCalls = 0;
};

class ScopedTimer {
public:
    explicit ScopedTimer(ScopeStat& stat) : m_stat(stat), m_start(std::chrono::steady_clock::now()) {}
    ~ScopedTimer() {
        const auto end = std::chrono::steady_clock::now();
        m_stat.add(static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_start).count()));
    }
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    ScopeStat& m_stat;
    std::chrono::steady_clock::time_point m_start;
};

struct ScopeSample {
    std::string_view name;
    double lastFrameMs = 0.0;   // total time in the previous frame (all threads)
    double averageMs = 0.0;     // exponential moving average
    u32 calls = 0;              // calls in the previous frame
};

// Snapshots and resets the per-frame accumulators. Call once per frame on the
// main thread (BR_PROFILE_FRAME does this).
void endFrame();
// Samples from the most recent endFrame(), sorted by average cost descending.
std::vector<ScopeSample> samples();

} // namespace br::profile
