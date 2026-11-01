#pragma once

#include "engine/core/Types.h"

#include <nlohmann/json.hpp>

namespace br::game {

struct FlashlightTuning {
    float secondsPerBattery = 420.0f;   // 7 minutes of continuous light
    float lowChargeThreshold = 0.15f;   // below this the beam starts to stutter
    float dimmingStart = 0.35f;         // output fades from here down to minOutput
    float minOutput = 0.35f;            // relative brightness just before empty
    float flickerRateHz = 0.6f;         // average stutter events per second when low
};

// Flashlight model (gameplay side). The renderer reads `output()` as the spot
// light intensity multiplier; audio reacts to events. Flicker is a
// deterministic function of time so saves/replays reproduce it.
class Flashlight {
public:
    explicit Flashlight(FlashlightTuning tuning = {});

    // Returns true if the state changed (for click sounds).
    bool setOn(bool on);
    bool toggle() { return setOn(!m_on); }
    bool isOn() const { return m_on; }

    void update(float dt);
    void insertBattery() { m_charge = 1.0f; }

    float charge() const { return m_charge; }
    bool empty() const { return m_charge <= 0.0f; }
    bool low() const { return m_charge > 0.0f && m_charge < m_tuning.lowChargeThreshold; }
    // 0..1 light output including dimming and low-battery stutter.
    float output() const { return m_output; }
    // True on frames where a low-battery stutter started (audio click/buzz).
    bool stutteredThisFrame() const { return m_stutterStarted; }

    nlohmann::json toJson() const { return {{"on", m_on}, {"charge", m_charge}}; }
    void fromJson(const nlohmann::json& j);

private:
    FlashlightTuning m_tuning;
    bool m_on = false;
    float m_charge = 1.0f;
    float m_output = 0.0f;
    double m_time = 0.0;
    bool m_inStutter = false;
    bool m_stutterStarted = false;
};

} // namespace br::game
