#pragma once

#include "engine/renderer/Camera.h"
#include "game/app/GameContext.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace br::game {

// A self-contained playable/test scene driven by the Game loop. Development
// scenes (lighting test room, physics playground, streamed level preview) and
// the real level session all implement this.
class GameScene {
public:
    virtual ~GameScene() = default;
    virtual bool build() = 0;
    virtual void fixedUpdate(float /*dt*/) {}
    // `alpha` is the fixed-step interpolation factor for rendering.
    virtual void update(float dt, float alpha, Camera& camera) = 0;
    // Called after the 3D frame, before UI (debug overlays).
    virtual void renderOverlay() {}
};

// Scene factory table (game/app/SceneRegistry.cpp). Each feature team adds one
// row for its scene; Game picks the scene from --scene / --level.
std::unique_ptr<GameScene> createScene(std::string_view name, GameContext& ctx);
std::vector<std::string> sceneNames();

} // namespace br::game
