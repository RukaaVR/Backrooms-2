#pragma once

#include "engine/core/Log.h"

#include <cstdlib>

namespace br::detail {
[[noreturn]] inline void assertFailed(const char* expr, const char* file, int line, const char* msg) {
    ::br::log::write(::br::log::Level::Fatal, "assert",
                     std::format("{}:{}: assertion '{}' failed: {}", file, line, expr, msg ? msg : ""));
    std::abort();
}
} // namespace br::detail

// BR_ASSERT is compiled out in release builds; BR_VERIFY always evaluates and
// aborts with a log message, for invariants whose violation would corrupt data.
#ifdef BR_DEBUG
#define BR_ASSERT(expr, ...)                                                                       \
    do {                                                                                           \
        if (!(expr)) ::br::detail::assertFailed(#expr, __FILE__, __LINE__, "" __VA_ARGS__);        \
    } while (0)
#else
#define BR_ASSERT(expr, ...) ((void)0)
#endif

#define BR_VERIFY(expr, ...)                                                                       \
    do {                                                                                           \
        if (!(expr)) ::br::detail::assertFailed(#expr, __FILE__, __LINE__, "" __VA_ARGS__);        \
    } while (0)
