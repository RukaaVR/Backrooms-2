#pragma once

#include "engine/core/Types.h"

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace br {

namespace fs = std::filesystem;

// Resolved directory layout. Nothing in the engine or game may hardcode an
// absolute path: everything is expressed relative to one of these roots.
struct Paths {
    fs::path executableDir; // directory containing the executable
    fs::path dataDir;       // <exe>/data  (read-only runtime data)
    fs::path shaderDir;     // <data>/shaders
    fs::path assetDir;      // <data>/assets
    fs::path userDir;       // per-user writable root (saves, settings, logs)
    fs::path saveDir;       // <user>/saves
    fs::path cacheDir;      // <user>/cache (procedural asset cache, safe to delete)
    fs::path screenshotDir; // <user>/screenshots

    // Resolves the layout from the executable location. `userOverride`, when
    // non-empty, replaces the per-user root (used by tests and the screenshot
    // harness to stay hermetic).
    static Paths resolve(const fs::path& userOverride = {});
    bool ensureWritableDirs() const;
};

namespace file {

std::optional<std::string> readText(const fs::path& path);
std::optional<std::vector<u8>> readBinary(const fs::path& path);

// Crash-safe replacement: writes to a sibling temp file, flushes it to disk and
// renames it over the destination. A reader never observes a partially written
// file, and an existing valid file is only replaced once the new one is complete.
bool writeAtomic(const fs::path& path, std::span<const u8> bytes);
bool writeAtomic(const fs::path& path, std::string_view text);

} // namespace file

} // namespace br
