#pragma once

#include "engine/core/Types.h"
#include "game/items/ItemCatalog.h"

#include <nlohmann/json.hpp>

#include <vector>

namespace br::game {

// Tuning values (rationale: docs/ITEMS_AND_SURVIVAL.md). Designed to create
// pressure without tedium: no hunger/thirst meters, stamina recovers fast,
// health regenerates slowly up to a floor so a lone mistake is not a slow death.
struct SurvivalTuning {
    float maxHealth = 100.0f;
    float maxStamina = 100.0f;
    float maxSanity = 100.0f;
    float maxOxygen = 100.0f;

    float sprintDrainPerSecond = 13.0f;   // ~7.7 s of continuous sprint from full
    float jumpStaminaCost = 7.0f;
    float staminaRegenPerSecond = 20.0f;
    float staminaRegenDelaySeconds = 0.9f;
    float exhaustedRecoverFraction = 0.30f; // after hitting zero, sprint unlocks at 30 %

    float baseSanityDrainPerMinute = 0.5f;  // multiplied by the level's oppressiveness
    float darknessSanityDrainPerMinute = 4.0f; // at darkness = 1
    float fearSanityDrainPerSecond = 1.2f;  // at fear = 1 (entity visible and close)
    float sanityRecoveryInSafetyPerMinute = 1.5f; // lit, calm, no threats, up to the soft cap
    float sanitySoftCap = 70.0f;            // passive recovery never exceeds this

    float oxygenDrainPerSecond = 100.0f / 38.0f; // ~38 s breath hold
    float oxygenRecoverPerSecond = 45.0f;
    float drowningDamagePerSecond = 9.0f;

    float bleedDamagePerSecond = 0.7f;
    float healthRegenDelaySeconds = 15.0f;
    float healthRegenPerSecond = 0.6f;
    float healthRegenCap = 50.0f;           // natural healing stops at half health

    float sanityLowThreshold = 40.0f;
    float sanityCriticalThreshold = 15.0f;
    float criticalSanityDamagePerSecond = 0.25f; // "the Backrooms wear you down"
};

// Environmental/gameplay context sampled once per fixed step.
struct SurvivalInputs {
    bool sprinting = false;
    bool jumped = false;
    float darkness = 0.0f;           // 0 well lit .. 1 pitch black (at player)
    float fear = 0.0f;               // 0 none .. 1 hostile entity in view and close
    bool headUnderwater = false;
    float levelOppressiveness = 1.0f; // per-level multiplier on base sanity drain
    bool safe = false;               // in a safe zone (e.g. M.E.G. outpost): sanity recovers
};

enum class DamageType : u8 { Generic, Entity, Fall, Drowning, Electric, Burn, Cold, Sanity };

enum class SurvivalEventType : u8 {
    Damaged,
    Healed,
    Exhausted,
    StaminaRecovered,
    SanityLow,
    SanityCritical,
    SanityRecovered,
    StartedDrowning,
    Bleeding,
    Died,
};

struct SurvivalEvent {
    SurvivalEventType type;
    float amount = 0.0f;
    DamageType damageType = DamageType::Generic;
};

struct SurvivalState {
    float health = 100.0f;
    float stamina = 100.0f;
    float sanity = 100.0f;
    float oxygen = 100.0f;
    bool bleeding = false;
    bool exhausted = false;
    bool alive = true;
    float timeSinceDamage = 1000.0f;
    float timeSinceStaminaUse = 1000.0f;
    float boostSecondsLeft = 0.0f;
    float boostRegenMultiplier = 0.0f;
    DamageType lastDamage = DamageType::Generic;
};

class SurvivalSystem {
public:
    explicit SurvivalSystem(SurvivalTuning tuning = {});

    void update(float dt, const SurvivalInputs& in);

    bool canSprint() const { return m_state.alive && !m_state.exhausted && m_state.stamina > 0.0f; }
    // Jump gating: returns false (and spends nothing) if not enough stamina.
    bool trySpendStamina(float amount);

    void damage(float amount, DamageType type);
    void heal(float amount);
    void restoreSanity(float amount);
    void drainSanity(float amount);
    void restoreStamina(float amount);
    void setBleeding(bool bleeding);
    void applyItemEffect(const ItemEffect& effect);
    void revive(); // checkpoint respawn: full stats

    const SurvivalState& state() const { return m_state; }
    const SurvivalTuning& tuning() const { return m_tuning; }
    float health01() const { return m_state.health / m_tuning.maxHealth; }
    float stamina01() const { return m_state.stamina / m_tuning.maxStamina; }
    float sanity01() const { return m_state.sanity / m_tuning.maxSanity; }
    float oxygen01() const { return m_state.oxygen / m_tuning.maxOxygen; }

    std::vector<SurvivalEvent> drainEvents();

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);

private:
    void emit(SurvivalEventType type, float amount = 0.0f, DamageType dt = DamageType::Generic);
    void updateSanityThresholds(float before);

    SurvivalTuning m_tuning;
    SurvivalState m_state;
    std::vector<SurvivalEvent> m_events;
};

} // namespace br::game
