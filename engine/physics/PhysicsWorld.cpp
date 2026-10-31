// TEMPORARY SCAFFOLD (Technical Director, wave 0).
// Minimal placeholder so every module links while the Jolt-backed
// implementation is developed in parallel. It performs no simulation.
// OWNER: Physics Engineer — replace this file entirely with the Jolt
// implementation. Do not build gameplay on its behaviour.
#include "engine/physics/PhysicsWorld.h"

#include <unordered_map>

namespace br::physics {

struct PhysicsWorld::Impl {
    struct Body {
        BodyDesc desc;
        Transform t;
        glm::vec3 v{0.0f};
    };
    std::unordered_map<u32, Body> bodies;
    u32 next = 0;
    bool init = false;
    std::vector<WaterVolume> water;
};

PhysicsWorld::PhysicsWorld() : m_impl(std::make_unique<Impl>()) {}
PhysicsWorld::~PhysicsWorld() = default;
bool PhysicsWorld::initialize(const PhysicsSettings&) { return m_impl->init = true; }
void PhysicsWorld::shutdown() { m_impl->bodies.clear(); m_impl->init = false; }
bool PhysicsWorld::initialized() const { return m_impl->init; }
void PhysicsWorld::step(float) {}
BodyId PhysicsWorld::createBody(const BodyDesc& d, bool) {
    const u32 id = m_impl->next++;
    m_impl->bodies[id] = {d, Transform{d.position, d.rotation, glm::vec3(1.0f)}, {}};
    return BodyId{id};
}
std::vector<BodyId> PhysicsWorld::createStaticBodies(const std::vector<BodyDesc>& ds) {
    std::vector<BodyId> out;
    for (const auto& d : ds) out.push_back(createBody(d));
    return out;
}
void PhysicsWorld::destroyBody(BodyId id) { m_impl->bodies.erase(id.value); }
void PhysicsWorld::destroyBodies(const std::vector<BodyId>& ids) { for (auto id : ids) destroyBody(id); }
bool PhysicsWorld::exists(BodyId id) const { return m_impl->bodies.count(id.value) != 0; }
Transform PhysicsWorld::transform(BodyId id) const {
    auto it = m_impl->bodies.find(id.value);
    return it == m_impl->bodies.end() ? Transform{} : it->second.t;
}
void PhysicsWorld::setTransform(BodyId id, glm::vec3 p, glm::quat r, bool) {
    if (auto it = m_impl->bodies.find(id.value); it != m_impl->bodies.end()) it->second.t = {p, r, glm::vec3(1.0f)};
}
void PhysicsWorld::moveKinematic(BodyId id, glm::vec3 p, glm::quat r, float) { setTransform(id, p, r); }
glm::vec3 PhysicsWorld::linearVelocity(BodyId) const { return {}; }
glm::vec3 PhysicsWorld::angularVelocity(BodyId) const { return {}; }
void PhysicsWorld::setLinearVelocity(BodyId, glm::vec3) {}
void PhysicsWorld::setAngularVelocity(BodyId, glm::vec3) {}
void PhysicsWorld::addImpulse(BodyId, glm::vec3) {}
void PhysicsWorld::addImpulseAt(BodyId, glm::vec3, glm::vec3) {}
void PhysicsWorld::addForce(BodyId, glm::vec3) {}
void PhysicsWorld::addTorque(BodyId, glm::vec3) {}
float PhysicsWorld::mass(BodyId) const { return 0.0f; }
void PhysicsWorld::setGravityFactor(BodyId, float) {}
void PhysicsWorld::setMotionType(BodyId, MotionType) {}
void PhysicsWorld::setLayer(BodyId, Layer) {}
bool PhysicsWorld::isActive(BodyId) const { return false; }
void PhysicsWorld::activate(BodyId) {}
u64 PhysicsWorld::userData(BodyId id) const {
    auto it = m_impl->bodies.find(id.value);
    return it == m_impl->bodies.end() ? 0 : it->second.desc.userData;
}
Surface PhysicsWorld::surface(BodyId) const { return Surface::Default; }
ConstraintId PhysicsWorld::createHinge(const HingeDesc&) { return {}; }
float PhysicsWorld::hingeAngle(ConstraintId) const { return 0.0f; }
void PhysicsWorld::setHingeTarget(ConstraintId, std::optional<float>, float) {}
void PhysicsWorld::destroyConstraint(ConstraintId) {}
std::optional<RayHit> PhysicsWorld::raycast(glm::vec3, glm::vec3, float, const QueryFilter&) const { return std::nullopt; }
std::optional<RayHit> PhysicsWorld::sphereCast(glm::vec3, float, glm::vec3, float, const QueryFilter&) const { return std::nullopt; }
std::vector<BodyId> PhysicsWorld::overlapSphere(glm::vec3, float, const QueryFilter&) const { return {}; }
std::vector<BodyId> PhysicsWorld::overlapBox(const AABB&, const QueryFilter&) const { return {}; }
bool PhysicsWorld::lineOfSight(glm::vec3, glm::vec3, const QueryFilter&) const { return true; }
void PhysicsWorld::setWaterVolumes(std::vector<WaterVolume> v) { m_impl->water = std::move(v); }
const std::vector<WaterVolume>& PhysicsWorld::waterVolumes() const { return m_impl->water; }
const WaterVolume* PhysicsWorld::waterAt(glm::vec3) const { return nullptr; }
std::vector<ContactEvent> PhysicsWorld::drainContactEvents() { return {}; }
std::vector<SensorEvent> PhysicsWorld::drainSensorEvents() { return {}; }
u32 PhysicsWorld::bodyCount() const { return static_cast<u32>(m_impl->bodies.size()); }
u32 PhysicsWorld::activeBodyCount() const { return 0; }

} // namespace br::physics
