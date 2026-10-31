#pragma once

#include "engine/physics/PhysicsTypes.h"

#include <memory>

namespace br::physics {

class PhysicsWorld;

struct CharacterSettings {
    float radius = 0.32f;
    float standingHeight = 1.80f;   // total capsule height
    float crouchingHeight = 1.10f;
    float maxSlopeDegrees = 46.0f;
    float maxStepHeight = 0.38f;    // stairs / kerbs climbed without jumping
    float mass = 75.0f;             // kg, used when pushing dynamic bodies
    float maxPushForce = 220.0f;    // N applied to dynamic bodies we walk into
    float skinWidth = 0.02f;
};

struct CharacterState {
    glm::vec3 position{0.0f};       // capsule bottom (feet)
    glm::vec3 velocity{0.0f};
    bool onGround = false;
    bool onSteepSlope = false;
    bool crouched = false;
    glm::vec3 groundNormal{0.0f, 1.0f, 0.0f};
    Surface groundSurface = Surface::Default;
    BodyId groundBody;
    glm::vec3 groundVelocity{0.0f}; // moving platforms / elevators
};

// Kinematic first-person capsule built on JPH::CharacterVirtual. The caller
// supplies the desired velocity each fixed step (from PlayerController, which
// owns acceleration/friction/game feel); this class resolves collisions,
// stairs, slopes, ground snapping and pushes dynamic props.
class CharacterController {
public:
    CharacterController();
    ~CharacterController();
    CharacterController(const CharacterController&) = delete;
    CharacterController& operator=(const CharacterController&) = delete;

    bool create(PhysicsWorld& world, const CharacterSettings& settings, glm::vec3 feetPosition);
    void destroy();
    bool valid() const;

    // Advances the character one fixed step. `desiredVelocity` is the full
    // velocity to attempt (including gravity / jump), in m/s.
    void update(float dt, glm::vec3 desiredVelocity, glm::vec3 gravity = {0.0f, -9.81f, 0.0f});

    // Attempts to change stance; returns false if there is no head clearance.
    bool setCrouched(bool crouched);
    bool canStandUp() const;

    void teleport(glm::vec3 feetPosition);
    const CharacterState& state() const;
    const CharacterSettings& settings() const;
    float currentHeight() const;
    // Eye height above the feet for the current stance (caller adds smoothing).
    float eyeHeight() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace br::physics
