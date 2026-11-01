#include "game/items/Flashlight.h"

#include "engine/core/Random.h"

#include <algorithm>
#include <cmath>

namespace br::game {

Flashlight::Flashlight(FlashlightTuning tuning) : m_tuning(tuning) {}

bool Flashlight::setOn(bool on) {
    if (on && empty()) {
        // Clicking a dead flashlight still makes a sound but produces no light.
        return true;
    }
    const bool changed = on != m_on;
    m_on = on;
    return changed;
}

void Flashlight::update(float dt) {
    m_time += dt;
    m_stutterStarted = false;
    if (!m_on) {
        m_output = 0.0f;
        m_inStutter = false;
        return;
    }
    m_charge = std::max(0.0f, m_charge - dt / m_tuning.secondsPerBattery);
    if (m_charge <= 0.0f) {
        m_on = false;
        m_output = 0.0f;
        return;
    }
    // Linear fade of output as the cell sags (perceptually: the beam yellows
    // and weakens before it dies, giving the player warning).
    float out = 1.0f;
    if (m_charge < m_tuning.dimmingStart) {
        const float t = m_charge / m_tuning.dimmingStart;
        out = m_tuning.minOutput + (1.0f - m_tuning.minOutput) * t;
    }
    // Low battery: deterministic stutter windows of 80 ms hashed from time.
    bool stutter = false;
    if (low()) {
        constexpr double kWindow = 0.08;
        const auto window = static_cast<i32>(m_time / kWindow);
        const float chance = m_tuning.flickerRateHz * static_cast<float>(kWindow) *
                             (1.0f + 2.0f * (1.0f - m_charge / m_tuning.lowChargeThreshold));
        stutter = hashToUnitFloat(hash2i(0xF1A5u, window, 0)) < chance;
    }
    if (stutter && !m_inStutter) {
        m_stutterStarted = true;
    }
    m_inStutter = stutter;
    m_output = stutter ? out * 0.08f : out;
}

void Flashlight::fromJson(const nlohmann::json& j) {
    if (!j.is_object()) {
        return;
    }
    m_on = j.value("on", false);
    m_charge = std::clamp(j.value("charge", 1.0f), 0.0f, 1.0f);
}

} // namespace br::game
