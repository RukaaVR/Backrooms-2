#include "game/levels/LevelDefinition.h"

#include "engine/core/Log.h"

#include <algorithm>

namespace br::levels {

void LevelRegistry::add(LevelDefinition def) {
    auto it = std::find_if(m_levels.begin(), m_levels.end(), [&](const auto& l) { return l.id == def.id; });
    if (it != m_levels.end()) {
        BR_LOG_WARN("levels", "Level '{}' registered twice; replacing", def.id);
        *it = std::move(def);
        return;
    }
    m_levels.push_back(std::move(def));
}

const LevelDefinition* LevelRegistry::find(std::string_view id) const {
    auto it = std::find_if(m_levels.begin(), m_levels.end(), [&](const auto& l) { return l.id == id; });
    return it == m_levels.end() ? nullptr : &*it;
}

} // namespace br::levels
