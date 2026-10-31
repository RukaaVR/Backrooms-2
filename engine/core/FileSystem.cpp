#include "engine/core/FileSystem.h"

#include "engine/core/Log.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>

#include <cstdio>
#include <fstream>
#include <random>
#include <system_error>

#if defined(_WIN32)
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace br {

Paths Paths::resolve(const fs::path& userOverride) {
    Paths p;
    if (const char* base = SDL_GetBasePath()) {
        p.executableDir = fs::path(base);
    } else {
        p.executableDir = fs::current_path();
    }
    p.dataDir = p.executableDir / "data";
    p.shaderDir = p.dataDir / "shaders";
    p.assetDir = p.dataDir / "assets";

    if (!userOverride.empty()) {
        p.userDir = userOverride;
    } else if (char* pref = SDL_GetPrefPath("BackroomsBeyondReality", "BackroomsBeyondReality")) {
        p.userDir = fs::path(pref);
        SDL_free(pref);
    } else {
        p.userDir = p.executableDir / "user_data";
    }
    p.saveDir = p.userDir / "saves";
    p.cacheDir = p.userDir / "cache";
    p.screenshotDir = p.userDir / "screenshots";
    return p;
}

bool Paths::ensureWritableDirs() const {
    std::error_code ec;
    for (const auto* dir : {&userDir, &saveDir, &cacheDir, &screenshotDir}) {
        fs::create_directories(*dir, ec);
        if (ec) {
            BR_LOG_ERROR("fs", "Cannot create directory '{}': {}", dir->string(), ec.message());
            return false;
        }
    }
    return true;
}

namespace file {

std::optional<std::string> readText(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    std::string s;
    in.seekg(0, std::ios::end);
    const auto size = in.tellg();
    if (size < 0) {
        return std::nullopt;
    }
    s.resize(static_cast<usize>(size));
    in.seekg(0, std::ios::beg);
    in.read(s.data(), static_cast<std::streamsize>(s.size()));
    if (!in && !in.eof()) {
        return std::nullopt;
    }
    return s;
}

std::optional<std::vector<u8>> readBinary(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    in.seekg(0, std::ios::end);
    const auto size = in.tellg();
    if (size < 0) {
        return std::nullopt;
    }
    std::vector<u8> data(static_cast<usize>(size));
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return data;
}

bool writeAtomic(const fs::path& path, std::span<const u8> bytes) {
    std::error_code ec;
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path(), ec);
    }
    std::random_device rd;
    const fs::path tmp = path.string() + ".tmp" + std::to_string(rd() & 0xFFFFu);

    FILE* f = std::fopen(tmp.string().c_str(), "wb");
    if (!f) {
        BR_LOG_ERROR("fs", "Cannot open '{}' for writing", tmp.string());
        return false;
    }
    bool ok = bytes.empty() || std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size();
    ok = ok && std::fflush(f) == 0;
#if defined(_WIN32)
    ok = ok && _commit(_fileno(f)) == 0;
#else
    ok = ok && ::fsync(fileno(f)) == 0;
#endif
    ok = (std::fclose(f) == 0) && ok;
    if (!ok) {
        BR_LOG_ERROR("fs", "Failed writing '{}'", tmp.string());
        fs::remove(tmp, ec);
        return false;
    }
    // rename() over an existing file is atomic on POSIX and on NTFS through
    // MoveFileEx(MOVEFILE_REPLACE_EXISTING), which std::filesystem uses.
    fs::rename(tmp, path, ec);
    if (ec) {
        BR_LOG_ERROR("fs", "Atomic rename '{}' -> '{}' failed: {}", tmp.string(), path.string(), ec.message());
        fs::remove(tmp, ec);
        return false;
    }
    return true;
}

bool writeAtomic(const fs::path& path, std::string_view text) {
    return writeAtomic(path, std::span<const u8>(reinterpret_cast<const u8*>(text.data()), text.size()));
}

} // namespace file
} // namespace br
