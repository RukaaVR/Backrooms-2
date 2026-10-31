#pragma once

#include "engine/core/FileSystem.h"

namespace br {
class Window;
class InputSystem;
class JobSystem;
class MainThreadQueue;
namespace render {
class Renderer;
}
namespace physics {
class PhysicsWorld;
}
namespace audio {
class AudioSystem;
}
namespace materials {
class MaterialRegistry;
}
namespace levels {
class LevelRegistry;
}
} // namespace br

namespace br::game {

struct Settings;

// Explicit dependency bundle handed to game systems instead of globals. The
// Game object owns every referenced system and outlives all users.
struct GameContext {
    Paths paths;
    Settings& settings;
    Window& window;
    InputSystem& input;
    JobSystem& jobs;
    MainThreadQueue& mainThread;
    render::Renderer& renderer;
    physics::PhysicsWorld& physics;
    audio::AudioSystem& audio;
    materials::MaterialRegistry& materials;
    levels::LevelRegistry& levels;
};

} // namespace br::game
