#include "game/inventory/Inventory.h"

#include "engine/core/Log.h"

#include <algorithm>

namespace br::game {

Inventory::Inventory(const ItemCatalog& catalog, int capacitySlots)
    : m_catalog(catalog), m_capacity(std::max(1, capacitySlots)) {}

AddResult Inventory::add(std::string_view itemId, int count, const nlohmann::json& data) {
    AddResult result;
    if (count <= 0) {
        return result;
    }
    const auto def = m_catalog.find(itemId);
    if (!def) {
        BR_LOG_WARN("inventory", "Unknown item '{}'", itemId);
        result.rejected = count;
        return result;
    }
    if (def->category == ItemCategory::Key || def->category == ItemCategory::Document) {
        auto& list = def->category == ItemCategory::Key ? m_keys : m_documents;
        if (std::find(list.begin(), list.end(), itemId) == list.end()) {
            list.emplace_back(itemId);
        }
        result.added = count;
        return result;
    }

    int remaining = count;
    const int maxStack = std::max(1, def->maxStack);
    // Items carrying per-instance data (e.g. a flashlight's charge) never merge.
    const bool mergeable = data.empty() || data.is_null();
    if (mergeable) {
        for (auto& s : m_slots) {
            if (remaining == 0) {
                break;
            }
            if (s.itemId == itemId && s.count < maxStack && s.data.empty()) {
                const int n = std::min(remaining, maxStack - s.count);
                s.count += n;
                remaining -= n;
            }
        }
    }
    while (remaining > 0 && usedSlots() < m_capacity) {
        const int n = std::min(remaining, maxStack);
        m_slots.push_back({std::string(itemId), n, data.is_null() ? nlohmann::json::object() : data});
        remaining -= n;
    }
    result.added = count - remaining;
    result.rejected = remaining;
    return result;
}

int Inventory::remove(std::string_view itemId, int count) {
    int removed = 0;
    // Take from the last stacks first so partially used stacks drain predictably.
    for (int i = usedSlots() - 1; i >= 0 && removed < count; --i) {
        ItemStack& s = m_slots[static_cast<usize>(i)];
        if (s.itemId != itemId) {
            continue;
        }
        const int n = std::min(count - removed, s.count);
        s.count -= n;
        removed += n;
        if (s.count == 0) {
            m_slots.erase(m_slots.begin() + i);
        }
    }
    return removed;
}

int Inventory::removeFromSlot(int index, int count) {
    if (index < 0 || index >= usedSlots() || count <= 0) {
        return 0;
    }
    ItemStack& s = m_slots[static_cast<usize>(index)];
    const int n = std::min(count, s.count);
    s.count -= n;
    if (s.count == 0) {
        m_slots.erase(m_slots.begin() + index);
    }
    return n;
}

int Inventory::count(std::string_view itemId) const {
    if (ItemCatalog::isKeyId(itemId)) {
        return hasKey(itemId) ? 1 : 0;
    }
    if (ItemCatalog::isDocumentId(itemId)) {
        return hasDocument(itemId) ? 1 : 0;
    }
    int n = 0;
    for (const auto& s : m_slots) {
        if (s.itemId == itemId) {
            n += s.count;
        }
    }
    return n;
}

bool Inventory::canFit(std::string_view itemId, int count) const {
    const auto def = m_catalog.find(itemId);
    if (!def) {
        return false;
    }
    if (def->category == ItemCategory::Key || def->category == ItemCategory::Document) {
        return true;
    }
    const int maxStack = std::max(1, def->maxStack);
    int space = (m_capacity - usedSlots()) * maxStack;
    for (const auto& s : m_slots) {
        if (s.itemId == itemId && s.data.empty()) {
            space += maxStack - s.count;
        }
    }
    return space >= count;
}

bool Inventory::hasKey(std::string_view keyId) const {
    return std::find(m_keys.begin(), m_keys.end(), keyId) != m_keys.end();
}

bool Inventory::hasDocument(std::string_view documentId) const {
    return std::find(m_documents.begin(), m_documents.end(), documentId) != m_documents.end();
}

ItemStack* Inventory::slot(int index) {
    return (index >= 0 && index < usedSlots()) ? &m_slots[static_cast<usize>(index)] : nullptr;
}

void Inventory::setCapacity(int slots) { m_capacity = std::max(1, slots); }

float Inventory::totalMassKg() const {
    float m = 0.0f;
    for (const auto& s : m_slots) {
        if (const auto def = m_catalog.find(s.itemId)) {
            m += def->massKg * static_cast<float>(s.count);
        }
    }
    return m;
}

nlohmann::json Inventory::toJson() const {
    nlohmann::json slots = nlohmann::json::array();
    for (const auto& s : m_slots) {
        slots.push_back({{"id", s.itemId}, {"count", s.count}, {"data", s.data}});
    }
    return {{"capacity", m_capacity}, {"slots", slots}, {"keys", m_keys}, {"documents", m_documents}};
}

bool Inventory::fromJson(const nlohmann::json& j) {
    if (!j.is_object()) {
        return false;
    }
    clear();
    if (auto it = j.find("capacity"); it != j.end() && it->is_number_integer()) {
        setCapacity(it->get<int>());
    }
    if (auto it = j.find("slots"); it != j.end() && it->is_array()) {
        for (const auto& s : *it) {
            if (!s.is_object() || !s.contains("id") || !s["id"].is_string()) {
                continue;
            }
            const std::string id = s["id"].get<std::string>();
            if (!m_catalog.contains(id)) {
                BR_LOG_WARN("inventory", "Dropping unknown item '{}' from save", id);
                continue;
            }
            const int count = s.value("count", 1);
            const nlohmann::json data = s.contains("data") ? s["data"] : nlohmann::json::object();
            if (count > 0) {
                m_slots.push_back({id, count, data});
            }
        }
    }
    auto readList = [&](const char* key, std::vector<std::string>& out) {
        if (auto it = j.find(key); it != j.end() && it->is_array()) {
            for (const auto& v : *it) {
                if (v.is_string()) {
                    out.push_back(v.get<std::string>());
                }
            }
        }
    };
    readList("keys", m_keys);
    readList("documents", m_documents);
    return true;
}

void Inventory::clear() {
    m_slots.clear();
    m_keys.clear();
    m_documents.clear();
}

} // namespace br::game
