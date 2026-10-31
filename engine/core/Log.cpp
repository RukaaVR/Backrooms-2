#include "engine/core/Log.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <mutex>

namespace br::log {
namespace {

struct LogState {
    std::mutex mutex;
    std::ofstream file;
    std::atomic<Level> minLevel{
#ifdef BR_DEBUG
        Level::Debug
#else
        Level::Info
#endif
    };
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
};

LogState& state() {
    static LogState s;
    return s;
}

const char* levelName(Level level) {
    switch (level) {
    case Level::Trace: return "TRACE";
    case Level::Debug: return "DEBUG";
    case Level::Info: return "INFO ";
    case Level::Warn: return "WARN ";
    case Level::Error: return "ERROR";
    case Level::Fatal: return "FATAL";
    }
    return "?";
}

} // namespace

void setMinimumLevel(Level level) { state().minLevel.store(level); }
Level minimumLevel() { return state().minLevel.load(std::memory_order_relaxed); }

bool openLogFile(const std::filesystem::path& path) {
    auto& s = state();
    std::lock_guard lock(s.mutex);
    s.file.close();
    s.file.open(path, std::ios::out | std::ios::trunc);
    return s.file.is_open();
}

void closeLogFile() {
    auto& s = state();
    std::lock_guard lock(s.mutex);
    s.file.close();
}

void write(Level level, std::string_view category, std::string_view message) {
    auto& s = state();
    const double seconds =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - s.start).count();
    const std::string line =
        std::format("[{:9.3f}] {} [{}] {}\n", seconds, levelName(level), category, message);
    std::lock_guard lock(s.mutex);
    std::fputs(line.c_str(), stderr);
    if (s.file.is_open()) {
        s.file << line;
        if (level >= Level::Warn) {
            s.file.flush();
        }
    }
}

} // namespace br::log
