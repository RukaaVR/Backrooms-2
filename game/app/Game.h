#pragma once

#include "engine/audio/AudioSystem.h"
#include "engine/core/JobSystem.h"
#include "engine/core/Time.h"
#include "engine/input/Input.h"
#include "engine/materials/MaterialRegistry.h"
#include "engine/physics/PhysicsWorld.h"
#include "engine/platform/Window.h"
#include "engine/renderer/Camera.h"
#include "engine/renderer/Renderer.h"
#include "game/app/GameContext.h"
#include "game/app/LaunchOptions.h"
#include "game/levels/LevelDefinition.h"
#include "game/settings/Settings.h"

#include <memory>
#include <optional>

namespace br::game {

class DevScene;

enum class GameState { Boot, MainMenu, Loading, Playing, Paused, Quitting };

// Top-level application: owns every engine system, runs the frame loop and the
// high-level state machine. Feature systems receive a GameContext.
class Game {
public:
    Game();
    ~Game();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // Returns the process exit code.
    int run(const LaunchOptions& options);

private:
    bool initialize();
    void shutdown();
    void pumpEvents();
    void fixedUpdate(float dt);
    void update(float dt, float alpha);
    void render(float dt);
    void applySettings();
    void limitFrameRate(double frameStartSeconds);
    bool handleScreenshotRequests();

    LaunchOptions m_options;
    Settings m_settings;
    Paths m_paths;

    std::unique_ptr<PlatformContext> m_platform;
    Window m_window;
    InputSystem m_input;
    std::unique_ptr<JobSystem> m_jobs;
    MainThreadQueue m_mainThread;
    render::Renderer m_renderer;
    physics::PhysicsWorld m_physics;
    audio::AudioSystem m_audio;
    materials::MaterialRegistry m_materials;
    levels::LevelRegistry m_levels;
    std::unique_ptr<GameContext> m_context;

    std::unique_ptr<DevScene> m_devScene;
    Camera m_camera;

    GameState m_state = GameState::Boot;
    FixedTimestep m_fixed{1.0 / 60.0};
    Stopwatch m_clock;
    u64 m_frameIndex = 0;
    std::vector<double> m_frameTimes;
    bool m_running = true;
    int m_exitCode = 0;
};

} // namespace br::game
