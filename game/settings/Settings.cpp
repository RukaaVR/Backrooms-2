#include "game/settings/Settings.h"

#include "engine/core/Log.h"

#include <nlohmann/json.hpp>

#include <algorithm>

namespace br::game {
namespace {

template <typename T>
void read(const nlohmann::json& j, const char* key, T& out) {
    if (auto it = j.find(key); it != j.end()) {
        try {
            out = it->template get<T>();
        } catch (const nlohmann::json::exception& e) {
            BR_LOG_WARN("settings", "Ignoring invalid value for '{}': {}", key, e.what());
        }
    }
}

template <typename E>
void readEnum(const nlohmann::json& j, const char* key, E& out, int maxValue) {
    int v = static_cast<int>(out);
    read(j, key, v);
    out = static_cast<E>(std::clamp(v, 0, maxValue));
}

} // namespace

void Settings::applyPreset(GraphicsPreset preset) {
    graphics.preset = preset;
    auto& r = graphics.render;
    switch (preset) {
    case GraphicsPreset::Low:
        r.shadowQuality = 1;
        r.textureQuality = 0;
        r.ambientOcclusion = false;
        r.screenSpaceReflections = false;
        r.volumetricFog = false;
        r.bloom = true;
        r.antiAliasing = render::AntiAliasing::FXAA;
        r.anisotropy = 2.0f;
        r.renderScale = 0.75f;
        break;
    case GraphicsPreset::Medium:
        r.shadowQuality = 2;
        r.textureQuality = 1;
        r.ambientOcclusion = true;
        r.screenSpaceReflections = false;
        r.volumetricFog = true;
        r.bloom = true;
        r.antiAliasing = render::AntiAliasing::TAA;
        r.anisotropy = 4.0f;
        r.renderScale = 1.0f;
        break;
    case GraphicsPreset::High:
        r.shadowQuality = 2;
        r.textureQuality = 2;
        r.ambientOcclusion = true;
        r.screenSpaceReflections = true;
        r.volumetricFog = true;
        r.bloom = true;
        r.antiAliasing = render::AntiAliasing::TAA;
        r.anisotropy = 8.0f;
        r.renderScale = 1.0f;
        break;
    case GraphicsPreset::Ultra:
        r.shadowQuality = 3;
        r.textureQuality = 2;
        r.ambientOcclusion = true;
        r.screenSpaceReflections = true;
        r.volumetricFog = true;
        r.bloom = true;
        r.antiAliasing = render::AntiAliasing::TAA;
        r.anisotropy = 16.0f;
        r.renderScale = 1.0f;
        break;
    case GraphicsPreset::Custom:
        break;
    }
}

render::RenderSettings Settings::effectiveRenderSettings() const {
    render::RenderSettings r = graphics.render;
    r.brightness = accessibility.brightness;
    r.reducedFlashing = accessibility.reducedFlashing;
    r.foundFootage = gameplay.foundFootageFilter;
    return r;
}

nlohmann::json Settings::toJson() const {
    const auto& g = graphics;
    const auto& r = g.render;
    nlohmann::json j;
    j["version"] = kVersion;
    j["graphics"] = {
        {"width", g.resolutionWidth},
        {"height", g.resolutionHeight},
        {"fullscreen", static_cast<int>(g.fullscreen)},
        {"vsync", g.vsync},
        {"fps_limit", g.fpsLimit},
        {"preset", static_cast<int>(g.preset)},
        {"fov", g.fieldOfView},
        {"render_scale", r.renderScale},
        {"shadow_quality", r.shadowQuality},
        {"texture_quality", r.textureQuality},
        {"ambient_occlusion", r.ambientOcclusion},
        {"reflections", r.screenSpaceReflections},
        {"bloom", r.bloom},
        {"volumetric_fog", r.volumetricFog},
        {"antialiasing", static_cast<int>(r.antiAliasing)},
        {"anisotropy", r.anisotropy},
        {"film_grain", r.filmGrain},
        {"chromatic_aberration", r.chromaticAberration},
    };
    j["audio"] = {{"master", audio.master}, {"ambient", audio.ambient}, {"effects", audio.effects},
                  {"music", audio.music},   {"voice", audio.voice},     {"ui", audio.ui}};
    j["controls"] = {{"mouse_sensitivity", controls.mouseSensitivity},
                     {"invert_y", controls.invertY},
                     {"toggle_sprint", controls.toggleSprint},
                     {"toggle_crouch", controls.toggleCrouch},
                     {"bindings", controls.bindings.toJson()}};
    j["accessibility"] = {{"subtitles", accessibility.subtitles},
                          {"reduced_camera_motion", accessibility.reducedCameraMotion},
                          {"reduced_flashing", accessibility.reducedFlashing},
                          {"brightness", accessibility.brightness},
                          {"high_contrast_prompts", accessibility.highContrastPrompts},
                          {"ui_scale", accessibility.uiScale}};
    j["gameplay"] = {{"show_stamina", gameplay.showStaminaBar},
                     {"show_fps", gameplay.showFps},
                     {"found_footage", gameplay.foundFootageFilter}};
    return j;
}

bool Settings::fromJson(const nlohmann::json& j) {
    if (!j.is_object()) {
        return false;
    }
    if (auto it = j.find("graphics"); it != j.end() && it->is_object()) {
        const auto& g = *it;
        auto& r = graphics.render;
        read(g, "width", graphics.resolutionWidth);
        read(g, "height", graphics.resolutionHeight);
        readEnum(g, "fullscreen", graphics.fullscreen, 2);
        read(g, "vsync", graphics.vsync);
        read(g, "fps_limit", graphics.fpsLimit);
        readEnum(g, "preset", graphics.preset, 4);
        read(g, "fov", graphics.fieldOfView);
        read(g, "render_scale", r.renderScale);
        read(g, "shadow_quality", r.shadowQuality);
        read(g, "texture_quality", r.textureQuality);
        read(g, "ambient_occlusion", r.ambientOcclusion);
        read(g, "reflections", r.screenSpaceReflections);
        read(g, "bloom", r.bloom);
        read(g, "volumetric_fog", r.volumetricFog);
        readEnum(g, "antialiasing", r.antiAliasing, 2);
        read(g, "anisotropy", r.anisotropy);
        read(g, "film_grain", r.filmGrain);
        read(g, "chromatic_aberration", r.chromaticAberration);
        graphics.resolutionWidth = std::clamp(graphics.resolutionWidth, 640, 7680);
        graphics.resolutionHeight = std::clamp(graphics.resolutionHeight, 360, 4320);
        graphics.fpsLimit = std::clamp(graphics.fpsLimit, 0, 1000);
        graphics.fieldOfView = std::clamp(graphics.fieldOfView, 50.0f, 110.0f);
        r.renderScale = std::clamp(r.renderScale, 0.5f, 2.0f);
        r.shadowQuality = std::clamp(r.shadowQuality, 0, 3);
        r.textureQuality = std::clamp(r.textureQuality, 0, 2);
        r.anisotropy = std::clamp(r.anisotropy, 1.0f, 16.0f);
    }
    if (auto it = j.find("audio"); it != j.end() && it->is_object()) {
        for (auto [key, value] : {std::pair{"master", &audio.master}, {"ambient", &audio.ambient},
                                  {"effects", &audio.effects}, {"music", &audio.music}, {"voice", &audio.voice},
                                  {"ui", &audio.ui}}) {
            read(*it, key, *value);
            *value = std::clamp(*value, 0.0f, 1.0f);
        }
    }
    if (auto it = j.find("controls"); it != j.end() && it->is_object()) {
        read(*it, "mouse_sensitivity", controls.mouseSensitivity);
        controls.mouseSensitivity = std::clamp(controls.mouseSensitivity, 0.05f, 10.0f);
        read(*it, "invert_y", controls.invertY);
        read(*it, "toggle_sprint", controls.toggleSprint);
        read(*it, "toggle_crouch", controls.toggleCrouch);
        if (auto b = it->find("bindings"); b != it->end()) {
            controls.bindings.fromJson(*b);
        }
    }
    if (auto it = j.find("accessibility"); it != j.end() && it->is_object()) {
        read(*it, "subtitles", accessibility.subtitles);
        read(*it, "reduced_camera_motion", accessibility.reducedCameraMotion);
        read(*it, "reduced_flashing", accessibility.reducedFlashing);
        read(*it, "brightness", accessibility.brightness);
        read(*it, "high_contrast_prompts", accessibility.highContrastPrompts);
        read(*it, "ui_scale", accessibility.uiScale);
        accessibility.brightness = std::clamp(accessibility.brightness, 0.5f, 2.0f);
        accessibility.uiScale = std::clamp(accessibility.uiScale, 0.75f, 2.0f);
    }
    if (auto it = j.find("gameplay"); it != j.end() && it->is_object()) {
        read(*it, "show_stamina", gameplay.showStaminaBar);
        read(*it, "show_fps", gameplay.showFps);
        read(*it, "found_footage", gameplay.foundFootageFilter);
    }
    return true;
}

bool Settings::save(const fs::path& path) const { return file::writeAtomic(path, toJson().dump(2)); }

bool Settings::load(const fs::path& path) {
    const auto text = file::readText(path);
    if (!text) {
        return false;
    }
    const auto j = nlohmann::json::parse(*text, nullptr, false);
    if (j.is_discarded()) {
        BR_LOG_WARN("settings", "Settings file '{}' is corrupt; using defaults", path.string());
        return false;
    }
    return fromJson(j);
}

} // namespace br::game
