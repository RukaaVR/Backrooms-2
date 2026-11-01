#pragma once

#include "engine/core/Types.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace br::game {

enum class ItemCategory : u8 { Tool, Consumable, Key, Document, Component, Misc };

std::string_view categoryName(ItemCategory c);

// What happens when an item is used. Effects are applied by ItemUseSystem; the
// catalog itself is pure data.
struct ItemEffect {
    float health = 0.0f;          // +HP
    float sanity = 0.0f;          // +sanity points
    float stamina = 0.0f;         // +stamina points (instant)
    float staminaRegenBoost = 0.0f; // multiplier added to regen for boostSeconds
    float boostSeconds = 0.0f;
    bool rechargesFlashlight = false; // batteries
    bool stopsBleeding = false;
};

struct ItemDefinition {
    std::string id;               // canonical id (docs/CONTENT_IDS.md)
    std::string name;             // display name
    std::string description;      // inventory/examine text
    ItemCategory category = ItemCategory::Misc;
    int maxStack = 1;
    float massKg = 0.2f;          // used when dropped into the world as a physics prop
    bool usable = false;
    bool examinable = true;
    bool droppable = true;
    bool consumedOnUse = false;
    float useSeconds = 0.0f;      // time the use animation/action takes
    ItemEffect effect;
    std::string prefab;           // world model prefab id
    std::string useSound;         // audio bank id
    std::string pickupSound = "ui.pickup";
    std::string iconHint;         // procedural icon shape for the UI
};

// Registry of every item the game knows. Parameterised families use a
// prefix: "key_<name>" and "document_<name>" resolve against the "key_*" and
// "document_*" templates so content can introduce new keys/documents without
// code changes.
class ItemCatalog {
public:
    // Builds the catalog with all standard items (tuning per
    // docs/ITEMS_AND_SURVIVAL.md).
    static ItemCatalog createDefault();

    void add(ItemDefinition def);
    // Returns the definition, resolving templated families. The returned
    // definition for a templated id carries the concrete id.
    std::optional<ItemDefinition> find(std::string_view id) const;
    bool contains(std::string_view id) const { return find(id).has_value(); }
    const std::vector<ItemDefinition>& all() const { return m_items; }

    static bool isKeyId(std::string_view id) { return id.starts_with("key_"); }
    static bool isDocumentId(std::string_view id) { return id.starts_with("document_"); }

private:
    const ItemDefinition* findExact(std::string_view id) const;
    std::vector<ItemDefinition> m_items;
};

} // namespace br::game
