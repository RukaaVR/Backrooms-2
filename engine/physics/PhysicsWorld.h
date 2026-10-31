#pragma once

#include "engine/physics/PhysicsTypes.h"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace br::physics {

struct PhysicsSettings {
    u32 maxBodies = 65536;
    u32 maxBodyPairs = 65536;
    u32 maxContactConstraints = 32768;
    int collisionSteps = 1;          // sub-steps per fixed update
    glm::vec3 gravity{0.0f, -9.81f, 0.0f};
    u32 workerThreads = 2;           // Jolt job threads
};

// Handle to a joint/constraint (door hinges, held-object springs).
struct ConstraintId {
    u32 value = 0xFFFFFFFFu;
    bool valid() const { return value != 0xFFFFFFFFu; }
};

struct HingeDesc {
    BodyId body;                     // dynamic door leaf
    BodyId anchor;                   // invalid = attach to the world
    glm::vec3 pivot{0.0f};           // world-space hinge point
    glm::vec3 axis{0.0f, 1.0f, 0.0f};
    glm::vec3 normal{1.0f, 0.0f, 0.0f}; // reference direction at angle 0
    float minAngle = -1.9f;          // radians
    float maxAngle = 1.9f;
    float maxFrictionTorque = 4.0f;  // Nm, resists free swinging (door feel)
};

// Water volume for buoyancy: everything below `surfaceHeight` inside `bounds`.
struct WaterVolume {
    AABB bounds;
    float surfaceHeight = 0.0f;
    float density = 1000.0f;   // kg/m^3 (buoyancy force scale)
    float linearDrag = 0.6f;
    float angularDrag = 0.05f;
    glm::vec3 flow{0.0f};      // current velocity
};

// Facade over Jolt Physics. Owns the Jolt PhysicsSystem, allocators and job
// system. Simulation is stepped at a fixed rate by the game loop; all methods
// are main-thread only unless noted. Body positions are in metres, Y up.
class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();
    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    bool initialize(const PhysicsSettings& settings = {});
    void shutdown();
    bool initialized() const;

    void step(float dt);

    // --- bodies ----------------------------------------------------------------
    BodyId createBody(const BodyDesc& desc, bool activate = true);
    // Creates many static bodies in one broadphase insertion (chunk streaming).
    std::vector<BodyId> createStaticBodies(const std::vector<BodyDesc>& descs);
    void destroyBody(BodyId id);
    void destroyBodies(const std::vector<BodyId>& ids);
    bool exists(BodyId id) const;

    Transform transform(BodyId id) const;            // position + rotation (scale = 1)
    void setTransform(BodyId id, glm::vec3 position, glm::quat rotation, bool activate = true);
    void moveKinematic(BodyId id, glm::vec3 targetPosition, glm::quat targetRotation, float dt);
    glm::vec3 linearVelocity(BodyId id) const;
    glm::vec3 angularVelocity(BodyId id) const;
    void setLinearVelocity(BodyId id, glm::vec3 v);
    void setAngularVelocity(BodyId id, glm::vec3 w);
    void addImpulse(BodyId id, glm::vec3 impulse);
    void addImpulseAt(BodyId id, glm::vec3 impulse, glm::vec3 worldPoint);
    void addForce(BodyId id, glm::vec3 force);
    void addTorque(BodyId id, glm::vec3 torque);
    float mass(BodyId id) const;
    void setGravityFactor(BodyId id, float factor);
    void setMotionType(BodyId id, MotionType motion);
    void setLayer(BodyId id, Layer layer);
    bool isActive(BodyId id) const;
    void activate(BodyId id);
    u64 userData(BodyId id) const;
    Surface surface(BodyId id) const;

    // --- constraints ------------------------------------------------------------
    ConstraintId createHinge(const HingeDesc& desc);
    float hingeAngle(ConstraintId id) const;
    // Drives the hinge towards an angle with a velocity motor (scripted doors).
    void setHingeTarget(ConstraintId id, std::optional<float> targetAngle, float maxTorque = 50.0f);
    void destroyConstraint(ConstraintId id);

    // --- queries (thread-safe for reads between steps) -------------------------
    std::optional<RayHit> raycast(glm::vec3 origin, glm::vec3 direction, float maxDistance,
                                  const QueryFilter& filter = {}) const;
    std::optional<RayHit> sphereCast(glm::vec3 origin, float radius, glm::vec3 direction, float maxDistance,
                                     const QueryFilter& filter = {}) const;
    std::vector<BodyId> overlapSphere(glm::vec3 center, float radius, const QueryFilter& filter = {}) const;
    std::vector<BodyId> overlapBox(const AABB& box, const QueryFilter& filter = {}) const;
    // True if a clear line exists between a and b (AI vision, audio occlusion).
    bool lineOfSight(glm::vec3 a, glm::vec3 b, const QueryFilter& filter = {}) const;

    // --- water -----------------------------------------------------------------------
    void setWaterVolumes(std::vector<WaterVolume> volumes);
    const std::vector<WaterVolume>& waterVolumes() const;
    // Returns the volume containing p (for swimming / underwater audio).
    const WaterVolume* waterAt(glm::vec3 p) const;

    // --- events (drained once per frame by the game) ---------------------------
    std::vector<ContactEvent> drainContactEvents();
    std::vector<SensorEvent> drainSensorEvents();

    // Opaque access for CharacterController (same module).
    struct Impl;
    Impl* impl() { return m_impl.get(); }
    const Impl* impl() const { return m_impl.get(); }

    u32 bodyCount() const;
    u32 activeBodyCount() const;

private:
    std::unique_ptr<Impl> m_impl;
};

} // namespace br::physics
