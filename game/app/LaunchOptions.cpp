#include "game/app/LaunchOptions.h"

#include <charconv>
#include <cstdlib>
#include <string_view>

namespace br::game {
namespace {

std::optional<float> toFloat(std::string_view s) {
    try {
        return std::stof(std::string(s));
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<long long> toInt(std::string_view s) {
    long long v = 0;
    auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{} || p != s.data() + s.size()) {
        return std::nullopt;
    }
    return v;
}

} // namespace

LaunchOptions LaunchOptions::parse(int argc, char** argv) {
    LaunchOptions o;
    for (int i = 1; i < argc; ++i) {
        const std::string_view a = argv[i];
        auto next = [&]() -> std::string_view { return (i + 1 < argc) ? std::string_view(argv[++i]) : std::string_view(); };
        if (a == "--level") {
            o.level = next();
        } else if (a == "--seed") {
            if (auto v = toInt(next())) o.seed = static_cast<u64>(*v);
        } else if (a == "--width") {
            if (auto v = toInt(next())) o.width = static_cast<int>(*v);
        } else if (a == "--height") {
            if (auto v = toInt(next())) o.height = static_cast<int>(*v);
        } else if (a == "--hidden") {
            o.hiddenWindow = true;
        } else if (a == "--windowed") {
            o.windowed = true;
        } else if (a == "--no-vsync") {
            o.noVsync = true;
        } else if (a == "--debug-gl") {
            o.debugGL = true;
        } else if (a == "--user-dir") {
            o.userDir = fs::path(std::string(next()));
        } else if (a == "--screenshot") {
            o.screenshotPath = fs::path(std::string(next()));
        } else if (a == "--screenshot-suite") {
            o.screenshotSuiteDir = fs::path(std::string(next()));
        } else if (a == "--scene") {
            o.screenshotScene = next();
        } else if (a == "--warmup-frames") {
            if (auto v = toInt(next())) o.warmupFrames = static_cast<int>(*v);
        } else if (a == "--frames") {
            if (auto v = toInt(next())) o.exitAfterFrames = static_cast<int>(*v);
        } else if (a == "--camera") {
            auto x = toFloat(next()), y = toFloat(next()), z = toFloat(next());
            if (x && y && z) o.cameraPosition = glm::vec3(*x, *y, *z);
        } else if (a == "--yaw") {
            o.cameraYawDegrees = toFloat(next());
        } else if (a == "--pitch") {
            o.cameraPitchDegrees = toFloat(next());
        } else if (a == "--benchmark") {
            o.benchmark = true;
        } else {
            o.unknown.emplace_back(a);
        }
    }
    return o;
}

std::string LaunchOptions::usage() {
    return "Usage: BackroomsBeyondReality [options]\n"
           "  --level <id>             start directly in a level (level0, level1, ..., level37, levelfun)\n"
           "  --seed <n>               world seed\n"
           "  --width/--height <px>    window size\n"
           "  --windowed --hidden --no-vsync --debug-gl\n"
           "  --user-dir <path>        override saves/settings directory\n"
           "  --screenshot <file.png>  render warm-up frames, capture, exit\n"
           "  --screenshot-suite <dir> capture the deterministic visual test suite\n"
           "  --scene <name>           single suite scene\n"
           "  --warmup-frames <n>      frames before capture (default 30)\n"
           "  --frames <n>             exit after n frames\n"
           "  --camera <x> <y> <z> --yaw <deg> --pitch <deg>\n"
           "  --benchmark              print frame-time statistics on exit\n";
}

} // namespace br::game
