// TEMPORARY SCAFFOLD (Technical Director, wave 0). No collision; walks on y=0.
// OWNER: Physics Engineer — replace with the JPH::CharacterVirtual implementation.
#include "engine/physics/CharacterController.h"

#include "engine/physics/PhysicsWorld.h"

namespace br::physics {

struct CharacterController::Impl {
    CharacterSettings settings;
    CharacterState state;
    bool valid = false;
};

CharacterController::CharacterController() : m_impl(std::make_unique<Impl>()) {}
CharacterController::~CharacterController() = default;
bool CharacterController::create(PhysicsWorld&, const CharacterSettings& s, glm::vec3 feet) {
    m_impl->settings = s;
    m_impl->state.position = feet;
    return m_impl->valid = true;
}
void CharacterController::destroy() { m_impl->valid = false; }
bool CharacterController::valid() const { return m_impl->valid; }
void CharacterController::update(float dt, glm::vec3 v, glm::vec3) {
    auto& st = m_impl->state;
    st.position += v * dt;
    st.velocity = v;
    if (st.position.y <= 0.0f) {
        st.position.y = 0.0f;
        st.velocity.y = 0.0f;
        st.onGround = true;
    } else {
        st.onGround = false;
    }
}
bool CharacterController::setCrouched(bool c) { m_impl->state.crouched = c; return true; }
bool CharacterController::canStandUp() const { return true; }
void CharacterController::teleport(glm::vec3 p) { m_impl->state.position = p; }
const CharacterState& CharacterController::state() const { return m_impl->state; }
const CharacterSettings& CharacterController::settings() const { return m_impl->settings; }
float CharacterController::currentHeight() const {
    return m_impl->state.crouched ? m_impl->settings.crouchingHeight : m_impl->settings.standingHeight;
}
float CharacterController::eyeHeight() const { return currentHeight() - 0.12f; }

} // namespace br::physics
