#pragma once

#include "engine/renderer/Camera.h"
#include "engine/renderer/RenderTypes.h"
#include "game/app/GameContext.h"

#include <vector>

namespace br::game {

// Developer test scene: a single lit room with a free-fly camera. It exists so
// the renderer, materials and input can be exercised before streamed levels
// are wired in, and serves as a stable screenshot regression scene.
class DevScene {
public:
    explicit DevScene(GameContext& ctx);
    ~DevScene();

    void build();
    void update(float dt, Camera& camera);

private:
    GameContext& m_ctx;
    std::vector<render::InstanceHandle> m_instances;
    std::vector<render::LightHandle> m_lights;
    std::vector<render::MeshHandle> m_meshes;
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    glm::vec3 m_position{0.0f, 1.6f, 3.0f};
    float m_time = 0.0f;
};

} // namespace br::game
