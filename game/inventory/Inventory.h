#pragma once

#include "game/items/ItemCatalog.h"

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace br::game {

struct ItemStack {
    std::string itemId;
    int count = 0;
    nlohmann::json data = nlohmann::json::object(); // per-stack state (e.g. flashlight charge)
};

struct AddResult {
    int added = 0;
    int rejected = 0; // did not fit
    bool fullyAdded() const { return rejected == 0; }
};

// Player inventory: a fixed number of carry slots (stacked by item), plus a
// key ring and a document archive that never consume slots. Keeping progression
// items outside the slot limit means a full bag can never soft-lock the player.
class Inventory {
public:
    static constexpr int kDefaultCapacity = 8;

    explicit Inventory(const ItemCatalog& catalog, int capacitySlots = kDefaultCapacity);

    // Adds as many as fit (filling existing stacks first). Keys/documents always fit.
    AddResult add(std::string_view itemId, int count = 1, const nlohmann::json& data = nlohmann::json::object());
    // Removes up to `count`; returns how many were removed.
    int remove(std::string_view itemId, int count = 1);
    // Removes from one specific slot (inventory UI "drop" on a slot).
    int removeFromSlot(int slot, int count = 1);

    int count(std::string_view itemId) const;
    bool has(std::string_view itemId) const { return count(itemId) > 0; }
    bool canFit(std::string_view itemId, int count = 1) const;

    bool hasKey(std::string_view keyId) const;
    bool hasDocument(std::string_view documentId) const;
    const std::vector<std::string>& keys() const { return m_keys; }
    const std::vector<std::string>& documents() const { return m_documents; }

    const std::vector<ItemStack>& slots() const { return m_slots; }
    ItemStack* slot(int index);
    int capacity() const { return m_capacity; }
    int usedSlots() const { return static_cast<int>(m_slots.size()); }
    void setCapacity(int slots); // never drops items; excess remains until used
    float totalMassKg() const;

    nlohmann::json toJson() const;
    // Unknown item ids are dropped with a warning (content removed between versions).
    bool fromJson(const nlohmann::json& j);
    void clear();

private:
    const ItemCatalog& m_catalog;
    int m_capacity;
    std::vector<ItemStack> m_slots;
    std::vector<std::string> m_keys;
    std::vector<std::string> m_documents;
};

} // namespace br::game
