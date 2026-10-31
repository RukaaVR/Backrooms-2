#pragma once

#include "engine/core/FileSystem.h"
#include "engine/input/Input.h"
#include "engine/platform/Window.h"
#include "engine/renderer/RenderTypes.h"

#include <nlohmann/json_fwd.hpp>

namespace br::game {

enum class GraphicsPreset : u8 { Low, Medium, High, Ultra, Custom };

struct GraphicsSettings {
    int resolutionWidth = 1920;
    int resolutionHeight = 1080;
    FullscreenMode fullscreen = FullscreenMode::Windowed;
    bool vsync = true;
    int fpsLimit = 0; // 0 = unlimited
    GraphicsPreset preset = GraphicsPreset::High;
    float fieldOfView = 75.0f; // vertical degrees
    render::RenderSettings render;
};

struct AudioSettings {
    float master = 0.9f;
    float ambient = 1.0f;
    float effects = 1.0f;
    float music = 0.7f;
    float voice = 1.0f;
    float ui = 0.8f;
};

struct ControlSettings {
    float mouseSensitivity = 1.0f; // multiplier on 0.0022 rad/count
    bool invertY = false;
    bool toggleSprint = false;
    bool toggleCrouch = false;
    ActionBindings bindings;
};

struct AccessibilitySettings {
    bool subtitles = true;
    bool reducedCameraMotion = false;
    bool reducedFlashing = false;
    float brightness = 1.0f;
    bool highContrastPrompts = false;
    float uiScale = 1.0f;
};

struct GameplaySettings {
    bool showStaminaBar = true;
    bool showFps = false;
    bool foundFootageFilter = false;
};

struct Settings {
    static constexpr int kVersion = 1;

    GraphicsSettings graphics;
    AudioSettings audio;
    ControlSettings controls;
    AccessibilitySettings accessibility;
    GameplaySettings gameplay;

    // Applies a preset's quality values to graphics.render.
    void applyPreset(GraphicsPreset preset);
    // Effective renderer settings including accessibility overrides.
    render::RenderSettings effectiveRenderSettings() const;

    nlohmann::json toJson() const;
    // Missing/invalid fields keep their defaults; returns false on parse error.
    bool fromJson(const nlohmann::json& j);

    bool save(const fs::path& path) const;
    // Loads settings; on missing/corrupt files keeps defaults and returns false.
    bool load(const fs::path& path);
};

} // namespace br::game
