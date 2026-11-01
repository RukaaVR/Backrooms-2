#include "engine/core/Profiler.h"

#include <algorithm>
#include <mutex>

namespace br::profile {

// Registration happens once per call site; the registry mutex is never taken
// on the timing hot path.
struct Registry {
    std::mutex mutex;
    std::vector<ScopeStat*> stats;
    std::vector<ScopeSample> lastSamples;

    static Registry& instance() {
        static Registry r;
        return r;
    }
};

ScopeStat::ScopeStat(std::string_view name) : m_name(name) {
    auto& r = Registry::instance();
    std::lock_guard lock(r.mutex);
    r.stats.push_back(this);
}

void endFrame() {
    // Smoothing factor for the moving average: ~30 frame time constant keeps
    // the overlay readable while still reacting within half a second.
    constexpr double kAverageBlend = 1.0 / 30.0;
    auto& r = Registry::instance();
    std::lock_guard lock(r.mutex);
    r.lastSamples.clear();
    r.lastSamples.reserve(r.stats.size());
    for (ScopeStat* s : r.stats) {
        const u64 ns = s->m_ns.exchange(0, std::memory_order_relaxed);
        const u32 calls = s->m_calls.exchange(0, std::memory_order_relaxed);
        s->m_lastMs = static_cast<double>(ns) * 1e-6;
        s->m_lastCalls = calls;
        s->m_avgMs += (s->m_lastMs - s->m_avgMs) * kAverageBlend;
        r.lastSamples.push_back({s->m_name, s->m_lastMs, s->m_avgMs, calls});
    }
    std::sort(r.lastSamples.begin(), r.lastSamples.end(),
              [](const ScopeSample& a, const ScopeSample& b) { return a.averageMs > b.averageMs; });
}

std::vector<ScopeSample> samples() {
    auto& r = Registry::instance();
    std::lock_guard lock(r.mutex);
    return r.lastSamples;
}

} // namespace br::profile
