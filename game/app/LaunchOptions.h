#pragma once

#include "engine/core/FileSystem.h"
#include "engine/core/Types.h"

#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <vector>

namespace br::game {

// Command-line options. Everything here also serves automated testing: the
// screenshot harness and CI smoke tests drive the real executable with them.
struct LaunchOptions {
    std::string level;             // start directly in a level ("level0"), skipping the menu
    std::optional<u64> seed;       // world seed override (deterministic runs)
    std::optional<int> width, height;
    bool hiddenWindow = false;
    bool windowed = false;         // force windowed regardless of settings
    bool noVsync = false;
    bool debugGL = false;
    fs::path userDir;              // override per-user data dir (hermetic tests)
    fs::path screenshotPath;       // capture one frame after warm-up then exit
    fs::path screenshotSuiteDir;   // run the deterministic visual test suite
    std::string screenshotScene;   // single named scene from the suite
    int warmupFrames = 30;         // frames rendered before capture
    int exitAfterFrames = 0;       // >0: quit after N frames (smoke tests)
    std::optional<glm::vec3> cameraPosition;
    std::optional<float> cameraYawDegrees;
    std::optional<float> cameraPitchDegrees;
    bool benchmark = false;        // log frame timing statistics on exit
    std::vector<std::string> unknown;

    static LaunchOptions parse(int argc, char** argv);
    static std::string usage();
};

} // namespace br::game
