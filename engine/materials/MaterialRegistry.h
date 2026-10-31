#pragma once

#include "engine/renderer/RenderTypes.h"

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace br::render {
class Renderer;
}

namespace br::materials {

// Name -> GPU material lookup shared by level generation, props and gameplay.
// Generators reference materials by stable string ids ("level0.wallpaper");
// the registry resolves them when chunks are instantiated on the main thread.
// Unknown names resolve to a visible fallback material (and are logged once)
// so missing content is obvious rather than silently invisible.
class MaterialRegistry {
public:
    void setFallback(render::MaterialHandle fallback) { m_fallback = fallback; }
    void add(std::string name, render::MaterialHandle handle);
    bool contains(std::string_view name) const;
    render::MaterialHandle find(std::string_view name) const;
    render::MaterialHandle findOrFallback(std::string_view name) const;
    std::vector<std::string> names() const;

    // Creates a simple untextured material if `name` is not registered yet.
    // Used for prototype content and by tests.
    render::MaterialHandle ensureSolid(render::Renderer& renderer, const std::string& name, glm::vec3 linearColor,
                                       float roughness = 0.8f, float metallic = 0.0f, glm::vec3 emissive = {});

    void clear();

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, render::MaterialHandle> m_map;
    mutable std::unordered_map<std::string, bool> m_warned;
    render::MaterialHandle m_fallback;
};

} // namespace br::materials
