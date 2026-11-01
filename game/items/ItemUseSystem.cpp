#include "game/items/ItemUseSystem.h"

namespace br::game {

ItemUseSystem::ItemUseSystem(const ItemCatalog& catalog, Inventory& inventory, SurvivalSystem& survival,
                             Flashlight& flashlight)
    : m_catalog(catalog), m_inventory(inventory), m_survival(survival), m_flashlight(flashlight) {}

ItemUseResult ItemUseSystem::begin(const std::string& itemId) {
    if (m_active) {
        return {false, "Already busy", "ui.error"};
    }
    const auto def = m_catalog.find(itemId);
    if (!def || !def->usable) {
        return {false, "Cannot use that", "ui.error"};
    }
    if (!m_inventory.has(itemId)) {
        return {false, "You don't have " + def->name, "ui.error"};
    }
    if (!m_survival.state().alive) {
        return {false, "", ""};
    }
    // Refuse pointless uses so supplies are not wasted by accident.
    const auto& st = m_survival.state();
    const auto& tu = m_survival.tuning();
    const bool wantsHealth = def->effect.health > 0.0f || def->effect.stopsBleeding;
    const bool wantsSanity = def->effect.sanity > 0.0f;
    const bool wantsStamina = def->effect.stamina > 0.0f;
    const bool healthFull = st.health >= tu.maxHealth && !st.bleeding;
    const bool sanityFull = st.sanity >= tu.maxSanity;
    const bool staminaFull = st.stamina >= tu.maxStamina;
    if (def->consumedOnUse && !def->effect.rechargesFlashlight && (!wantsHealth || healthFull) &&
        (!wantsSanity || sanityFull) && (!wantsStamina || staminaFull)) {
        return {false, "You don't need that right now", "ui.error"};
    }
    if (def->effect.rechargesFlashlight && m_flashlight.charge() >= 0.999f) {
        return {false, "The flashlight is fully charged", "ui.error"};
    }
    if (def->useSeconds <= 0.0f) {
        return apply(*def);
    }
    m_active = Active{*def, 0.0f};
    return {true, "", def->useSound};
}

std::optional<ItemUseResult> ItemUseSystem::update(float dt) {
    if (!m_active) {
        return std::nullopt;
    }
    if (!m_survival.state().alive) {
        m_active.reset();
        return std::nullopt;
    }
    m_active->elapsed += dt;
    if (m_active->elapsed < m_active->def.useSeconds) {
        return std::nullopt;
    }
    const ItemDefinition def = m_active->def;
    m_active.reset();
    return apply(def);
}

void ItemUseSystem::cancel() { m_active.reset(); }

float ItemUseSystem::progress01() const {
    if (!m_active || m_active->def.useSeconds <= 0.0f) {
        return 0.0f;
    }
    return std::min(1.0f, m_active->elapsed / m_active->def.useSeconds);
}

const std::string& ItemUseSystem::activeItem() const {
    static const std::string kNone;
    return m_active ? m_active->def.id : kNone;
}

ItemUseResult ItemUseSystem::apply(const ItemDefinition& def) {
    if (def.id == "flashlight") {
        const bool wasOn = m_flashlight.isOn();
        m_flashlight.toggle();
        if (!wasOn && m_flashlight.empty()) {
            return {true, "The flashlight is dead", "flashlight.off"};
        }
        return {true, "", m_flashlight.isOn() ? "flashlight.on" : "flashlight.off"};
    }
    if (def.id == "lighter") {
        m_lighterLit = !m_lighterLit;
        return {true, "", "switch.click"};
    }
    if (def.consumedOnUse && m_inventory.remove(def.id, 1) != 1) {
        return {false, "You don't have " + def.name, "ui.error"};
    }
    if (def.effect.rechargesFlashlight) {
        m_flashlight.insertBattery();
    }
    m_survival.applyItemEffect(def.effect);
    return {true, "Used " + def.name, def.useSound};
}

} // namespace br::game
