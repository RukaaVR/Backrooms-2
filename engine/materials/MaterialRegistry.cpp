#include "engine/materials/MaterialRegistry.h"

#include "engine/core/Log.h"
#include "engine/renderer/Renderer.h"

namespace br::materials {

void MaterialRegistry::add(std::string name, render::MaterialHandle handle) {
    std::lock_guard lock(m_mutex);
    m_map[std::move(name)] = handle;
}

bool MaterialRegistry::contains(std::string_view name) const {
    std::lock_guard lock(m_mutex);
    return m_map.find(std::string(name)) != m_map.end();
}

render::MaterialHandle MaterialRegistry::find(std::string_view name) const {
    std::lock_guard lock(m_mutex);
    auto it = m_map.find(std::string(name));
    return it == m_map.end() ? render::MaterialHandle{} : it->second;
}

render::MaterialHandle MaterialRegistry::findOrFallback(std::string_view name) const {
    std::lock_guard lock(m_mutex);
    auto it = m_map.find(std::string(name));
    if (it != m_map.end()) {
        return it->second;
    }
    auto& warned = m_warned[std::string(name)];
    if (!warned) {
        warned = true;
        BR_LOG_WARN("materials", "Material '{}' is not registered; using fallback", name);
    }
    return m_fallback;
}

std::vector<std::string> MaterialRegistry::names() const {
    std::lock_guard lock(m_mutex);
    std::vector<std::string> out;
    out.reserve(m_map.size());
    for (const auto& [k, v] : m_map) {
        out.push_back(k);
    }
    return out;
}

render::MaterialHandle MaterialRegistry::ensureSolid(render::Renderer& renderer, const std::string& name,
                                                     glm::vec3 linearColor, float roughness, float metallic,
                                                     glm::vec3 emissive) {
    if (auto h = find(name)) {
        return h;
    }
    render::MaterialDesc d;
    d.name = name;
    d.baseColor = glm::vec4(linearColor, 1.0f);
    d.roughness = roughness;
    d.metallic = metallic;
    d.emissiveColor = emissive;
    d.emissiveIntensity = glm::length(emissive) > 0.0f ? 1.0f : 0.0f;
    const auto h = renderer.createMaterial(d);
    add(name, h);
    return h;
}

void MaterialRegistry::clear() {
    std::lock_guard lock(m_mutex);
    m_map.clear();
    m_warned.clear();
}

} // namespace br::materials
