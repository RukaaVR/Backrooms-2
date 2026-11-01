#include "game/items/ItemCatalog.h"

#include "engine/core/Log.h"

#include <algorithm>
#include <cctype>

namespace br::game {
namespace {

// "key_storage_room" -> "Storage Room"
std::string humanise(std::string_view slug) {
    std::string out;
    bool upper = true;
    for (char c : slug) {
        if (c == '_' || c == '-') {
            out += ' ';
            upper = true;
            continue;
        }
        out += upper ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c;
        upper = false;
    }
    return out;
}

} // namespace

std::string_view categoryName(ItemCategory c) {
    switch (c) {
    case ItemCategory::Tool: return "Tool";
    case ItemCategory::Consumable: return "Consumable";
    case ItemCategory::Key: return "Key";
    case ItemCategory::Document: return "Document";
    case ItemCategory::Component: return "Component";
    case ItemCategory::Misc: return "Misc";
    }
    return "Misc";
}

void ItemCatalog::add(ItemDefinition def) {
    auto it = std::find_if(m_items.begin(), m_items.end(), [&](const auto& d) { return d.id == def.id; });
    if (it != m_items.end()) {
        BR_LOG_WARN("items", "Item '{}' defined twice; replacing", def.id);
        *it = std::move(def);
        return;
    }
    m_items.push_back(std::move(def));
}

const ItemDefinition* ItemCatalog::findExact(std::string_view id) const {
    auto it = std::find_if(m_items.begin(), m_items.end(), [&](const auto& d) { return d.id == id; });
    return it == m_items.end() ? nullptr : &*it;
}

std::optional<ItemDefinition> ItemCatalog::find(std::string_view id) const {
    if (const ItemDefinition* d = findExact(id)) {
        return *d;
    }
    const char* family = isKeyId(id) ? "key_*" : (isDocumentId(id) ? "document_*" : nullptr);
    if (!family) {
        return std::nullopt;
    }
    const ItemDefinition* tmpl = findExact(family);
    if (!tmpl) {
        return std::nullopt;
    }
    ItemDefinition d = *tmpl;
    d.id = std::string(id);
    const std::string_view slug = id.substr(id.find('_') + 1);
    d.name = (isKeyId(id) ? "Key: " : "Document: ") + humanise(slug);
    return d;
}

ItemCatalog ItemCatalog::createDefault() {
    ItemCatalog c;
    // Tuning rationale lives in docs/ITEMS_AND_SURVIVAL.md. Values are chosen so
    // supplies matter without becoming a chore: a battery lasts several minutes
    // of continuous light, almond water is the primary sanity tool, medical
    // items are scarce but decisive.
    {
        ItemDefinition d;
        d.id = "flashlight";
        d.name = "Flashlight";
        d.description = "A heavy aluminium flashlight. The lens is scratched but the beam is strong.";
        d.category = ItemCategory::Tool;
        d.massKg = 0.45f;
        d.usable = true; // toggles the light
        d.prefab = "item.flashlight";
        d.useSound = "flashlight.on";
        d.iconHint = "flashlight";
        c.add(d);
    }
    {
        ItemDefinition d;
        d.id = "battery";
        d.name = "Battery";
        d.description = "A D-cell battery. Swaps into the flashlight, restoring a full charge.";
        d.category = ItemCategory::Consumable;
        d.maxStack = 6;
        d.massKg = 0.14f;
        d.usable = true;
        d.consumedOnUse = true;
        d.useSeconds = 1.2f;
        d.effect.rechargesFlashlight = true;
        d.prefab = "item.battery";
        d.useSound = "item.battery";
        d.iconHint = "battery";
        c.add(d);
    }
    {
        ItemDefinition d;
        d.id = "almond_water";
        d.name = "Almond Water";
        d.description = "A sealed bottle of faintly sweet water. Drinking it steadies the mind.";
        d.category = ItemCategory::Consumable;
        d.maxStack = 4;
        d.massKg = 0.55f;
        d.usable = true;
        d.consumedOnUse = true;
        d.useSeconds = 2.0f;
        d.effect.sanity = 35.0f;
        d.effect.stamina = 20.0f;
        d.effect.health = 5.0f;
        d.prefab = "item.almond_water";
        d.useSound = "item.drink";
        d.iconHint = "bottle";
        c.add(d);
    }
    {
        ItemDefinition d;
        d.id = "medkit";
        d.name = "First Aid Kit";
        d.description = "A sealed first aid kit: dressings, antiseptic, painkillers.";
        d.category = ItemCategory::Consumable;
        d.maxStack = 2;
        d.massKg = 0.8f;
        d.usable = true;
        d.consumedOnUse = true;
        d.useSeconds = 3.5f;
        d.effect.health = 60.0f;
        d.effect.stopsBleeding = true;
        d.prefab = "item.medkit";
        d.useSound = "paper.rustle";
        d.iconHint = "cross";
        c.add(d);
    }
    {
        ItemDefinition d;
        d.id = "bandage";
        d.name = "Bandage";
        d.description = "A roll of elastic bandage. Stops bleeding and patches minor wounds.";
        d.category = ItemCategory::Consumable;
        d.maxStack = 5;
        d.massKg = 0.05f;
        d.usable = true;
        d.consumedOnUse = true;
        d.useSeconds = 2.0f;
        d.effect.health = 20.0f;
        d.effect.stopsBleeding = true;
        d.prefab = "item.bandage";
        d.useSound = "paper.rustle";
        d.iconHint = "bandage";
        c.add(d);
    }
    {
        ItemDefinition d;
        d.id = "energy_bar";
        d.name = "Energy Bar";
        d.description = "Stale but edible. Briefly makes running easier.";
        d.category = ItemCategory::Consumable;
        d.maxStack = 4;
        d.massKg = 0.06f;
        d.usable = true;
        d.consumedOnUse = true;
        d.useSeconds = 1.5f;
        d.effect.stamina = 40.0f;
        d.effect.staminaRegenBoost = 0.5f;
        d.effect.boostSeconds = 60.0f;
        d.prefab = "item.energy_bar";
        d.useSound = "paper.rustle";
        d.iconHint = "bar";
        c.add(d);
    }
    {
        ItemDefinition d;
        d.id = "fuse";
        d.name = "Fuse";
        d.description = "A ceramic cartridge fuse rated for heavy current. Fits industrial panels.";
        d.category = ItemCategory::Component;
        d.maxStack = 3;
        d.massKg = 0.1f;
        d.prefab = "item.fuse";
        d.iconHint = "fuse";
        c.add(d);
    }
    {
        ItemDefinition d;
        d.id = "crowbar";
        d.name = "Crowbar";
        d.description = "Forged steel. Pries open jammed doors and stuck panels.";
        d.category = ItemCategory::Tool;
        d.massKg = 1.6f;
        d.prefab = "item.crowbar";
        d.iconHint = "crowbar";
        c.add(d);
    }
    {
        ItemDefinition d;
        d.id = "lighter";
        d.name = "Lighter";
        d.description = "A brass petrol lighter. A small, warm light that never needs batteries.";
        d.category = ItemCategory::Tool;
        d.massKg = 0.06f;
        d.usable = true;
        d.prefab = "item.lighter";
        d.useSound = "switch.click";
        d.iconHint = "flame";
        c.add(d);
    }
    {
        ItemDefinition d;
        d.id = "key_*";
        d.name = "Key";
        d.description = "A worn key with a paper tag.";
        d.category = ItemCategory::Key;
        d.massKg = 0.02f;
        d.droppable = false; // keys are progression items; never lose them
        d.prefab = "item.key";
        d.pickupSound = "ui.pickup";
        d.iconHint = "key";
        c.add(d);
    }
    {
        ItemDefinition d;
        d.id = "document_*";
        d.name = "Document";
        d.description = "A note left by someone who came before you.";
        d.category = ItemCategory::Document;
        d.massKg = 0.01f;
        d.droppable = false;
        d.prefab = "item.paper";
        d.pickupSound = "paper.rustle";
        d.iconHint = "paper";
        c.add(d);
    }
    return c;
}

} // namespace br::game
