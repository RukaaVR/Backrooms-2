#pragma once

#include "engine/core/FileSystem.h"

#include <random>
#include <string>

namespace br::test {

// Unique scratch directory under the system temp dir, removed on destruction.
class TempDir {
public:
    TempDir() {
        std::random_device rd;
        m_path = fs::temp_directory_path() / ("br_test_" + std::to_string(rd()) + std::to_string(rd()));
        fs::create_directories(m_path);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(m_path, ec);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    const fs::path& path() const { return m_path; }

private:
    fs::path m_path;
};

} // namespace br::test
