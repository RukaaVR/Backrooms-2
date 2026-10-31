#include "game/app/DevScene.h"

#include "engine/input/Input.h"
#include "engine/materials/MaterialRegistry.h"
#include "engine/renderer/Renderer.h"
#include "engine/scene/MeshData.h"
#include "game/settings/Settings.h"

#include <SDL3/SDL_scancode.h>

namespace br::game {

DevScene::DevScene(GameContext& ctx) : m_ctx(ctx) {}

DevScene::~DevScene() {
    for (auto h : m_instances) m_ctx.renderer.destroyInstance(h);
    for (auto h : m_lights) m_ctx.renderer.destroyLight(h);
    for (auto h : m_meshes) m_ctx.renderer.destroyMesh(h);
}

void DevScene::build() {
    auto& r = m_ctx.renderer;
    auto& reg = m_ctx.materials;
    // Placeholder solid materials (the material library replaces these ids
    // with procedural PBR materials when it registers them first).
    const auto wall = reg.ensureSolid(r, "level0.wallpaper", {0.62f, 0.52f, 0.25f}, 0.85f);
    const auto carpet = reg.ensureSolid(r, "level0.carpet", {0.42f, 0.36f, 0.20f}, 0.95f);
    const auto ceiling = reg.ensureSolid(r, "level0.ceiling_tile", {0.75f, 0.72f, 0.62f}, 0.9f);
    const auto panel = reg.ensureSolid(r, "level0.light_panel", {0.9f, 0.9f, 0.85f}, 0.4f, 0.0f, {6.0f, 6.0f, 5.6f});
    const auto pillar = reg.ensureSolid(r, "level0.pillar", {0.62f, 0.52f, 0.25f}, 0.85f);

    const AABB room{{-6.0f, 0.0f, -8.0f}, {6.0f, 2.8f, 8.0f}};
    auto addMesh = [&](const MeshData& m, render::MaterialHandle mat) {
        const auto mesh = r.createMesh(m);
        m_meshes.push_back(mesh);
        render::InstanceDesc d;
        d.mesh = mesh;
        d.material = mat;
        m_instances.push_back(r.createInstance(d));
    };

    MeshData walls, floor, ceil;
    mesh::addBox(walls, room, 1.0f, {true, true, false, false, true, true}, true);
    mesh::addBox(floor, room, 1.0f, {false, false, false, true, false, false}, true);
    mesh::addBox(ceil, room, 1.0f, {false, false, true, false, false, false}, true);
    addMesh(walls, wall);
    addMesh(floor, carpet);
    addMesh(ceil, ceiling);

    MeshData pillars;
    for (float x : {-2.5f, 2.5f}) {
        mesh::addBox(pillars, AABB{{x - 0.35f, 0.0f, -0.35f}, {x + 0.35f, 2.8f, 0.35f}}, 1.0f,
                     {true, true, false, false, true, true});
    }
    addMesh(pillars, pillar);

    MeshData panels;
    for (float x : {-3.0f, 0.0f, 3.0f}) {
        for (float z : {-5.0f, -1.5f, 2.0f, 5.5f}) {
            mesh::addBox(panels, AABB{{x - 0.3f, 2.78f, z - 0.6f}, {x + 0.3f, 2.8f, z + 0.6f}}, 1.0f,
                         {false, false, false, true, false, false});
            render::LightDesc l;
            l.type = render::LightType::Rect;
            l.position = {x, 2.76f, z};
            l.direction = {0.0f, -1.0f, 0.0f};
            l.rectSize = {0.6f, 1.2f};
            l.color = render::colorTemperatureToRgb(4300.0f);
            l.intensity = 3.0f;
            l.range = 9.0f;
            m_lights.push_back(r.createLight(l));
        }
    }
    addMesh(panels, panel);

    render::EnvironmentDesc env;
    env.ambientColor = glm::vec3(0.035f, 0.03f, 0.018f);
    env.fogColor = glm::vec3(0.30f, 0.26f, 0.14f);
    env.fogDensity = 0.01f;
    env.exposureCompensation = 0.0f;
    r.setEnvironment(env);
}

void DevScene::update(float dt, Camera& camera) {
    m_time += dt;
    auto& in = m_ctx.input;
    const float sens = 0.0022f * m_ctx.settings.controls.mouseSensitivity;
    if (m_ctx.window.relativeMouse()) {
        m_yaw -= in.mouseDelta().x * sens;
        m_pitch -= in.mouseDelta().y * sens * (m_ctx.settings.controls.invertY ? -1.0f : 1.0f);
        m_pitch = glm::clamp(m_pitch, -1.5f, 1.5f);
    }
    camera.orientation = Camera::orientationFromYawPitch(m_yaw, m_pitch);
    glm::vec3 move(0.0f);
    if (in.actionDown(Action::MoveForward)) move += camera.forward();
    if (in.actionDown(Action::MoveBackward)) move -= camera.forward();
    if (in.actionDown(Action::MoveRight)) move += camera.right();
    if (in.actionDown(Action::MoveLeft)) move -= camera.right();
    if (glm::length(move) > 0.0f) {
        const float speed = in.actionDown(Action::Sprint) ? 6.0f : 2.5f;
        m_position += glm::normalize(move) * speed * dt;
    }
    camera.position = m_position;
}

} // namespace br::game
