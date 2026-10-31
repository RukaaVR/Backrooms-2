#include "game/app/Game.h"

#include "engine/core/Log.h"
#include "engine/resources/Image.h"
#include "game/app/DevScene.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <numeric>
#include <thread>

namespace br::game {

Game::Game() = default;
Game::~Game() { shutdown(); }

int Game::run(const LaunchOptions& options) {
    m_options = options;
    if (!initialize()) {
        shutdown();
        return 1;
    }

    double last = m_clock.elapsedSeconds();
    while (m_running) {
        const double frameStart = m_clock.elapsedSeconds();
        const double dt = std::min(frameStart - last, 0.25);
        last = frameStart;

        pumpEvents();
        if (!m_running) {
            break;
        }

        const int steps = m_fixed.advance(dt);
        for (int i = 0; i < steps; ++i) {
            fixedUpdate(static_cast<float>(m_fixed.step()));
        }
        update(static_cast<float>(dt), m_fixed.alpha());
        render(static_cast<float>(dt));

        if (handleScreenshotRequests()) {
            break;
        }
        m_window.swapBuffers();
        ++m_frameIndex;
        if (m_frameIndex > 1) {
            m_frameTimes.push_back(dt * 1000.0);
        }
        if (m_options.exitAfterFrames > 0 && m_frameIndex >= static_cast<u64>(m_options.exitAfterFrames)) {
            m_running = false;
        }
        limitFrameRate(frameStart);
    }

    if (m_options.benchmark && !m_frameTimes.empty()) {
        std::vector<double> sorted = m_frameTimes;
        std::sort(sorted.begin(), sorted.end());
        const double avg = std::accumulate(sorted.begin(), sorted.end(), 0.0) / static_cast<double>(sorted.size());
        const double p99 = sorted[std::min(sorted.size() - 1, static_cast<usize>(static_cast<double>(sorted.size()) * 0.99))];
        BR_LOG_INFO("bench", "frames={} avg={:.2f}ms ({:.1f} fps) p50={:.2f}ms p99={:.2f}ms max={:.2f}ms", sorted.size(), avg,
                    1000.0 / avg, sorted[sorted.size() / 2], p99, sorted.back());
    }
    shutdown();
    return m_exitCode;
}

bool Game::initialize() {
    m_paths = Paths::resolve(m_options.userDir);
    if (!m_paths.ensureWritableDirs()) {
        return false;
    }
    log::openLogFile(m_paths.userDir / "backrooms.log");
    BR_LOG_INFO("game", "THE BACKROOMS: BEYOND REALITY starting");
    BR_LOG_INFO("game", "Data: {} | User: {}", m_paths.dataDir.string(), m_paths.userDir.string());
    for (const auto& u : m_options.unknown) {
        BR_LOG_WARN("game", "Unknown option '{}'\n{}", u, LaunchOptions::usage());
    }

    m_settings.load(m_paths.userDir / "settings.json");

    m_platform = std::make_unique<PlatformContext>();
    if (!m_platform->ok()) {
        return false;
    }

    WindowConfig wc;
    wc.width = m_options.width.value_or(m_settings.graphics.resolutionWidth);
    wc.height = m_options.height.value_or(m_settings.graphics.resolutionHeight);
    wc.fullscreen = m_options.windowed || m_options.hiddenWindow ? FullscreenMode::Windowed : m_settings.graphics.fullscreen;
    wc.vsync = m_settings.graphics.vsync && !m_options.noVsync;
    wc.hidden = m_options.hiddenWindow;
    wc.debugContext = m_options.debugGL;
    if (!m_window.create(wc)) {
        return false;
    }

    m_jobs = std::make_unique<JobSystem>();

    if (!m_renderer.initialize(m_paths, m_window.drawableSize(), m_settings.effectiveRenderSettings())) {
        return false;
    }
    m_materials.setFallback(
        m_materials.ensureSolid(m_renderer, "engine.missing", {0.8f, 0.05f, 0.6f}, 0.5f));

    if (!m_physics.initialize()) {
        BR_LOG_ERROR("game", "Physics initialisation failed");
        return false;
    }
    if (!m_audio.initialize()) {
        BR_LOG_WARN("game", "Audio unavailable; continuing silently");
    }

    m_context = std::make_unique<GameContext>(GameContext{m_paths, m_settings, m_window, m_input, *m_jobs, m_mainThread,
                                                          m_renderer, m_physics, m_audio, m_materials, m_levels});
    applySettings();

    m_devScene = std::make_unique<DevScene>(*m_context);
    m_devScene->build();
    m_state = GameState::Playing;
    if (!m_options.hiddenWindow) {
        m_window.setRelativeMouse(true);
    }
    return true;
}

void Game::shutdown() {
    if (m_state == GameState::Quitting && !m_context) {
        return;
    }
    m_devScene.reset();
    m_context.reset();
    m_audio.shutdown();
    m_physics.shutdown();
    m_renderer.shutdown();
    m_jobs.reset();
    m_window.destroy();
    m_platform.reset();
    if (m_state != GameState::Boot) {
        m_settings.save(m_paths.userDir / "settings.json");
    }
    m_state = GameState::Quitting;
    log::closeLogFile();
}

void Game::applySettings() {
    m_renderer.applySettings(m_settings.effectiveRenderSettings());
    m_input.bindings() = m_settings.controls.bindings;
    using audio::Category;
    m_audio.setCategoryVolume(Category::Master, m_settings.audio.master);
    m_audio.setCategoryVolume(Category::Ambient, m_settings.audio.ambient);
    m_audio.setCategoryVolume(Category::Effects, m_settings.audio.effects);
    m_audio.setCategoryVolume(Category::Music, m_settings.audio.music);
    m_audio.setCategoryVolume(Category::Voice, m_settings.audio.voice);
    m_audio.setCategoryVolume(Category::UI, m_settings.audio.ui);
    m_camera.fovYDegrees = m_settings.graphics.fieldOfView;
}

void Game::pumpEvents() {
    m_input.beginFrame();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        m_window.handleEvent(e);
        m_input.processEvent(e);
    }
    if (m_window.closeRequested()) {
        m_running = false;
    }
    if (m_window.wasResized()) {
        m_renderer.resize(m_window.drawableSize());
    }
    // Temporary until the menu system lands: Pause toggles mouse capture.
    if (m_input.actionPressed(Action::Pause)) {
        m_window.setRelativeMouse(!m_window.relativeMouse());
    }
}

void Game::fixedUpdate(float dt) {
    if (m_state == GameState::Playing) {
        m_physics.step(dt);
    }
}

void Game::update(float dt, float /*alpha*/) {
    m_mainThread.drain(4.0);
    if (m_devScene) {
        m_devScene->update(dt, m_camera);
    }
    if (m_options.cameraPosition) {
        m_camera.position = *m_options.cameraPosition;
    }
    if (m_options.cameraYawDegrees || m_options.cameraPitchDegrees) {
        m_camera.orientation = Camera::orientationFromYawPitch(glm::radians(m_options.cameraYawDegrees.value_or(0.0f)),
                                                               glm::radians(m_options.cameraPitchDegrees.value_or(0.0f)));
    }
    m_audio.update(dt);
}

void Game::render(float dt) { m_renderer.renderFrame(m_camera, dt); }

bool Game::handleScreenshotRequests() {
    if (m_options.screenshotPath.empty() || m_frameIndex + 1 < static_cast<u64>(std::max(1, m_options.warmupFrames))) {
        return false;
    }
    glFinish();
    if (auto img = m_renderer.captureFrame()) {
        if (img->savePng(m_options.screenshotPath)) {
            BR_LOG_INFO("game", "Screenshot written to {}", m_options.screenshotPath.string());
        } else {
            BR_LOG_ERROR("game", "Failed to write screenshot {}", m_options.screenshotPath.string());
            m_exitCode = 2;
        }
    }
    m_running = false;
    return true;
}

void Game::limitFrameRate(double frameStart) {
    const int limit = m_settings.graphics.fpsLimit;
    if (limit <= 0) {
        return;
    }
    const double target = 1.0 / static_cast<double>(limit);
    // Sleep most of the remaining time, spin the last millisecond for precision.
    for (;;) {
        const double remaining = target - (m_clock.elapsedSeconds() - frameStart);
        if (remaining <= 0.0) {
            break;
        }
        if (remaining > 0.002) {
            std::this_thread::sleep_for(std::chrono::duration<double>(remaining - 0.001));
        }
    }
}

} // namespace br::game
