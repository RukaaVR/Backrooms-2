#include "game/survival/SurvivalSystem.h"

#include <algorithm>

namespace br::game {

SurvivalSystem::SurvivalSystem(SurvivalTuning tuning) : m_tuning(tuning) {
    m_state.health = m_tuning.maxHealth;
    m_state.stamina = m_tuning.maxStamina;
    m_state.sanity = m_tuning.maxSanity;
    m_state.oxygen = m_tuning.maxOxygen;
}

void SurvivalSystem::emit(SurvivalEventType type, float amount, DamageType dt) {
    m_events.push_back({type, amount, dt});
}

std::vector<SurvivalEvent> SurvivalSystem::drainEvents() {
    std::vector<SurvivalEvent> out;
    out.swap(m_events);
    return out;
}

void SurvivalSystem::update(float dt, const SurvivalInputs& in) {
    if (!m_state.alive || dt <= 0.0f) {
        return;
    }
    auto& s = m_state;
    const auto& t = m_tuning;
    s.timeSinceDamage += dt;
    s.timeSinceStaminaUse += dt;

    // --- stamina ----------------------------------------------------------------
    if (in.jumped) {
        trySpendStamina(t.jumpStaminaCost);
    }
    if (in.sprinting && canSprint()) {
        s.stamina = std::max(0.0f, s.stamina - t.sprintDrainPerSecond * dt);
        s.timeSinceStaminaUse = 0.0f;
        if (s.stamina <= 0.0f) {
            s.exhausted = true;
            emit(SurvivalEventType::Exhausted);
        }
    } else if (s.timeSinceStaminaUse >= t.staminaRegenDelaySeconds) {
        float regen = t.staminaRegenPerSecond;
        if (s.boostSecondsLeft > 0.0f) {
            regen *= 1.0f + s.boostRegenMultiplier;
        }
        s.stamina = std::min(t.maxStamina, s.stamina + regen * dt);
        if (s.exhausted && s.stamina >= t.maxStamina * t.exhaustedRecoverFraction) {
            s.exhausted = false;
            emit(SurvivalEventType::StaminaRecovered);
        }
    }
    s.boostSecondsLeft = std::max(0.0f, s.boostSecondsLeft - dt);

    // --- oxygen -------------------------------------------------------------------
    if (in.headUnderwater) {
        const bool hadAir = s.oxygen > 0.0f;
        s.oxygen = std::max(0.0f, s.oxygen - t.oxygenDrainPerSecond * dt);
        if (s.oxygen <= 0.0f) {
            if (hadAir) {
                emit(SurvivalEventType::StartedDrowning);
            }
            damage(t.drowningDamagePerSecond * dt, DamageType::Drowning);
            if (!s.alive) {
                return;
            }
        }
    } else {
        s.oxygen = std::min(t.maxOxygen, s.oxygen + t.oxygenRecoverPerSecond * dt);
    }

    // --- sanity -------------------------------------------------------------------
    const float sanityBefore = s.sanity;
    float drainPerSecond = t.baseSanityDrainPerMinute * in.levelOppressiveness / 60.0f +
                           t.darknessSanityDrainPerMinute * std::clamp(in.darkness, 0.0f, 1.0f) / 60.0f +
                           t.fearSanityDrainPerSecond * std::clamp(in.fear, 0.0f, 1.0f);
    if (in.safe && in.fear <= 0.0f) {
        drainPerSecond = 0.0f;
        if (s.sanity < t.sanitySoftCap) {
            s.sanity = std::min(t.sanitySoftCap, s.sanity + t.sanityRecoveryInSafetyPerMinute / 60.0f * dt);
        }
    }
    s.sanity = std::max(0.0f, s.sanity - drainPerSecond * dt);
    updateSanityThresholds(sanityBefore);
    if (s.sanity <= t.sanityCriticalThreshold) {
        damage(t.criticalSanityDamagePerSecond * dt, DamageType::Sanity);
        if (!s.alive) {
            return;
        }
    }

    // --- health -------------------------------------------------------------------
    if (s.bleeding) {
        damage(t.bleedDamagePerSecond * dt, DamageType::Generic);
        if (!s.alive) {
            return;
        }
    } else if (s.timeSinceDamage >= t.healthRegenDelaySeconds && s.health < t.healthRegenCap) {
        s.health = std::min(t.healthRegenCap, s.health + t.healthRegenPerSecond * dt);
    }
}

bool SurvivalSystem::trySpendStamina(float amount) {
    if (!m_state.alive || m_state.stamina < amount) {
        return false;
    }
    m_state.stamina -= amount;
    m_state.timeSinceStaminaUse = 0.0f;
    return true;
}

void SurvivalSystem::damage(float amount, DamageType type) {
    if (!m_state.alive || amount <= 0.0f) {
        return;
    }
    m_state.health = std::max(0.0f, m_state.health - amount);
    m_state.lastDamage = type;
    // Continuous trickle damage (bleeding, sanity) must not reset the regen
    // timer every frame, otherwise it would also block natural healing forever.
    if (amount > 0.5f || type == DamageType::Drowning) {
        m_state.timeSinceDamage = 0.0f;
        emit(SurvivalEventType::Damaged, amount, type);
    }
    if (m_state.health <= 0.0f) {
        m_state.alive = false;
        emit(SurvivalEventType::Died, 0.0f, type);
    }
}

void SurvivalSystem::heal(float amount) {
    if (!m_state.alive || amount <= 0.0f) {
        return;
    }
    m_state.health = std::min(m_tuning.maxHealth, m_state.health + amount);
    emit(SurvivalEventType::Healed, amount);
}

void SurvivalSystem::updateSanityThresholds(float before) {
    const float now = m_state.sanity;
    const auto& t = m_tuning;
    if (before > t.sanityLowThreshold && now <= t.sanityLowThreshold) {
        emit(SurvivalEventType::SanityLow, now);
    }
    if (before > t.sanityCriticalThreshold && now <= t.sanityCriticalThreshold) {
        emit(SurvivalEventType::SanityCritical, now);
    }
    if (before <= t.sanityLowThreshold && now > t.sanityLowThreshold) {
        emit(SurvivalEventType::SanityRecovered, now);
    }
}

void SurvivalSystem::restoreSanity(float amount) {
    if (!m_state.alive || amount <= 0.0f) {
        return;
    }
    const float before = m_state.sanity;
    m_state.sanity = std::min(m_tuning.maxSanity, m_state.sanity + amount);
    updateSanityThresholds(before);
}

void SurvivalSystem::drainSanity(float amount) {
    if (!m_state.alive || amount <= 0.0f) {
        return;
    }
    const float before = m_state.sanity;
    m_state.sanity = std::max(0.0f, m_state.sanity - amount);
    updateSanityThresholds(before);
}

void SurvivalSystem::restoreStamina(float amount) {
    m_state.stamina = std::min(m_tuning.maxStamina, m_state.stamina + std::max(0.0f, amount));
    if (m_state.exhausted && m_state.stamina >= m_tuning.maxStamina * m_tuning.exhaustedRecoverFraction) {
        m_state.exhausted = false;
        emit(SurvivalEventType::StaminaRecovered);
    }
}

void SurvivalSystem::setBleeding(bool bleeding) {
    if (bleeding && !m_state.bleeding) {
        emit(SurvivalEventType::Bleeding);
    }
    m_state.bleeding = bleeding;
}

void SurvivalSystem::applyItemEffect(const ItemEffect& e) {
    if (e.health > 0.0f) {
        heal(e.health);
    }
    if (e.sanity > 0.0f) {
        restoreSanity(e.sanity);
    }
    if (e.stamina > 0.0f) {
        restoreStamina(e.stamina);
    }
    if (e.boostSeconds > 0.0f) {
        m_state.boostSecondsLeft = std::max(m_state.boostSecondsLeft, e.boostSeconds);
        m_state.boostRegenMultiplier = std::max(m_state.boostRegenMultiplier, e.staminaRegenBoost);
    }
    if (e.stopsBleeding) {
        m_state.bleeding = false;
    }
}

void SurvivalSystem::revive() {
    m_state = SurvivalState{};
    m_state.health = m_tuning.maxHealth;
    m_state.stamina = m_tuning.maxStamina;
    m_state.sanity = std::max(m_tuning.sanitySoftCap, m_state.sanity);
    m_state.oxygen = m_tuning.maxOxygen;
}

nlohmann::json SurvivalSystem::toJson() const {
    const auto& s = m_state;
    return {{"health", s.health},   {"stamina", s.stamina},       {"sanity", s.sanity},
            {"oxygen", s.oxygen},   {"bleeding", s.bleeding},     {"exhausted", s.exhausted},
            {"alive", s.alive},     {"boost_s", s.boostSecondsLeft}, {"boost_mul", s.boostRegenMultiplier}};
}

void SurvivalSystem::fromJson(const nlohmann::json& j) {
    if (!j.is_object()) {
        return;
    }
    auto& s = m_state;
    auto clampTo = [](float v, float hi) { return std::clamp(v, 0.0f, hi); };
    s.health = clampTo(j.value("health", s.health), m_tuning.maxHealth);
    s.stamina = clampTo(j.value("stamina", s.stamina), m_tuning.maxStamina);
    s.sanity = clampTo(j.value("sanity", s.sanity), m_tuning.maxSanity);
    s.oxygen = clampTo(j.value("oxygen", s.oxygen), m_tuning.maxOxygen);
    s.bleeding = j.value("bleeding", false);
    s.exhausted = j.value("exhausted", false);
    s.alive = j.value("alive", true) && s.health > 0.0f;
    s.boostSecondsLeft = std::max(0.0f, j.value("boost_s", 0.0f));
    s.boostRegenMultiplier = std::max(0.0f, j.value("boost_mul", 0.0f));
}

} // namespace br::game
