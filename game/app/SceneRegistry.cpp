#include "engine/core/Log.h"
#include "game/app/DevScene.h"
#include "game/app/GameScene.h"

#include <array>
#include <functional>

namespace br::game {
namespace {

struct SceneEntry {
    std::string_view name;
    std::string_view description;
    std::unique_ptr<GameScene> (*create)(GameContext&);
};

// One row per scene. Keep rows sorted by owner to minimise merge conflicts.
const std::array kScenes = {
    SceneEntry{"dev", "Lighting/material test room with free-fly camera (Technical Director)",
               [](GameContext& c) -> std::unique_ptr<GameScene> { return std::make_unique<DevScene>(c); }},
};

} // namespace

std::unique_ptr<GameScene> createScene(std::string_view name, GameContext& ctx) {
    for (const auto& e : kScenes) {
        if (e.name == name) {
            return e.create(ctx);
        }
    }
    BR_LOG_ERROR("scene", "Unknown scene '{}'", name);
    return nullptr;
}

std::vector<std::string> sceneNames() {
    std::vector<std::string> out;
    for (const auto& e : kScenes) {
        out.emplace_back(e.name);
    }
    return out;
}

} // namespace br::game
