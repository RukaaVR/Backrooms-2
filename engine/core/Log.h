#pragma once

#include <filesystem>
#include <format>
#include <string>
#include <string_view>

namespace br::log {

enum class Level { Trace, Debug, Info, Warn, Error, Fatal };

// Thread-safe sink: writes to stderr and (optionally) a log file in the user
// data directory. Formatting happens on the calling thread; only the write is
// serialised.
void setMinimumLevel(Level level);
Level minimumLevel();
bool openLogFile(const std::filesystem::path& path);
void closeLogFile();
void write(Level level, std::string_view category, std::string_view message);

template <typename... Args>
void logf(Level level, std::string_view category, std::format_string<Args...> fmt, Args&&... args) {
    if (level < minimumLevel()) {
        return;
    }
    write(level, category, std::format(fmt, std::forward<Args>(args)...));
}

} // namespace br::log

#define BR_LOG_TRACE(cat, ...) ::br::log::logf(::br::log::Level::Trace, cat, __VA_ARGS__)
#define BR_LOG_DEBUG(cat, ...) ::br::log::logf(::br::log::Level::Debug, cat, __VA_ARGS__)
#define BR_LOG_INFO(cat, ...) ::br::log::logf(::br::log::Level::Info, cat, __VA_ARGS__)
#define BR_LOG_WARN(cat, ...) ::br::log::logf(::br::log::Level::Warn, cat, __VA_ARGS__)
#define BR_LOG_ERROR(cat, ...) ::br::log::logf(::br::log::Level::Error, cat, __VA_ARGS__)
#define BR_LOG_FATAL(cat, ...) ::br::log::logf(::br::log::Level::Fatal, cat, __VA_ARGS__)
