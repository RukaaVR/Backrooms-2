#pragma once

#include "game/inventory/Inventory.h"
#include "game/items/Flashlight.h"
#include "game/survival/SurvivalSystem.h"

#include <optional>
#include <string>

namespace br::game {

struct ItemUseResult {
    bool success = false;
    std::string message;  // HUD toast text
    std::string sound;    // audio bank id ("" = none)
};

// Timed item usage. Using an item takes `useSeconds` (drinking, bandaging);
// the action can be cancelled (e.g. by sprinting or taking a hit) and only
// consumes the item when it completes. Drives the HUD progress ring and the
// first-person animation.
class ItemUseSystem {
public:
    ItemUseSystem(const ItemCatalog& catalog, Inventory& inventory, SurvivalSystem& survival, Flashlight& flashlight);

    // Starts using an item; instant items complete immediately.
    ItemUseResult begin(const std::string& itemId);
    // Advances the current action; returns the result when it completes.
    std::optional<ItemUseResult> update(float dt);
    void cancel();

    bool busy() const { return m_active.has_value(); }
    float progress01() const;
    const std::string& activeItem() const;
    // The lighter is a battery-free fallback light source; the level session
    // spawns a small flickering warm point light at the hand while it is lit.
    bool lighterLit() const { return m_lighterLit; }
    void extinguishLighter() { m_lighterLit = false; }

private:
    ItemUseResult apply(const ItemDefinition& def);

    struct Active {
        ItemDefinition def;
        float elapsed = 0.0f;
    };

    const ItemCatalog& m_catalog;
    Inventory& m_inventory;
    SurvivalSystem& m_survival;
    Flashlight& m_flashlight;
    std::optional<Active> m_active;
    bool m_lighterLit = false;
};

} // namespace br::game
