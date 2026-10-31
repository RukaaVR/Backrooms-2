#include "engine/input/Input.h"

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include <string>

namespace br {
namespace {

constexpr std::array<std::string_view, static_cast<usize>(Action::Count)> kActionNames = {
    "move_forward", "move_backward", "move_left", "move_right", "jump",       "crouch",    "sprint",       "interact",
    "throw",        "flashlight",    "inventory", "use_item",   "pause",      "quick_save", "debug_console"};

} // namespace

std::string_view actionName(Action a) { return kActionNames[static_cast<usize>(a)]; }

std::optional<Action> actionFromName(std::string_view name) {
    for (usize i = 0; i < kActionNames.size(); ++i) {
        if (kActionNames[i] == name) {
            return static_cast<Action>(i);
        }
    }
    return std::nullopt;
}

std::string InputBinding::toString() const {
    switch (device) {
    case Device::Keyboard: {
        const char* n = SDL_GetScancodeName(static_cast<SDL_Scancode>(code));
        return (n && *n) ? std::string(n) : "Key" + std::to_string(code);
    }
    case Device::Mouse:
        return "Mouse" + std::to_string(code);
    default:
        return "";
    }
}

InputBinding InputBinding::fromString(std::string_view s) {
    if (s.empty()) {
        return {};
    }
    if (s.starts_with("Mouse")) {
        return mouse(std::stoi(std::string(s.substr(5))));
    }
    if (s.starts_with("Key")) {
        return key(std::stoi(std::string(s.substr(3))));
    }
    const SDL_Scancode sc = SDL_GetScancodeFromName(std::string(s).c_str());
    return sc == SDL_SCANCODE_UNKNOWN ? InputBinding{} : key(sc);
}

ActionBindings::ActionBindings() { resetToDefaults(); }

void ActionBindings::resetToDefaults() {
    for (auto& slots : m_bindings) {
        slots = {};
    }
    auto set = [&](Action a, InputBinding p, InputBinding s = {}) {
        m_bindings[static_cast<usize>(a)] = {p, s};
    };
    set(Action::MoveForward, InputBinding::key(SDL_SCANCODE_W), InputBinding::key(SDL_SCANCODE_UP));
    set(Action::MoveBackward, InputBinding::key(SDL_SCANCODE_S), InputBinding::key(SDL_SCANCODE_DOWN));
    set(Action::MoveLeft, InputBinding::key(SDL_SCANCODE_A), InputBinding::key(SDL_SCANCODE_LEFT));
    set(Action::MoveRight, InputBinding::key(SDL_SCANCODE_D), InputBinding::key(SDL_SCANCODE_RIGHT));
    set(Action::Jump, InputBinding::key(SDL_SCANCODE_SPACE));
    set(Action::Crouch, InputBinding::key(SDL_SCANCODE_LCTRL), InputBinding::key(SDL_SCANCODE_C));
    set(Action::Sprint, InputBinding::key(SDL_SCANCODE_LSHIFT));
    set(Action::Interact, InputBinding::key(SDL_SCANCODE_E), InputBinding::mouse(SDL_BUTTON_LEFT));
    set(Action::Throw, InputBinding::mouse(SDL_BUTTON_RIGHT), InputBinding::key(SDL_SCANCODE_G));
    set(Action::Flashlight, InputBinding::key(SDL_SCANCODE_F));
    set(Action::Inventory, InputBinding::key(SDL_SCANCODE_TAB), InputBinding::key(SDL_SCANCODE_I));
    set(Action::UseItem, InputBinding::key(SDL_SCANCODE_Q));
    set(Action::Pause, InputBinding::key(SDL_SCANCODE_ESCAPE), InputBinding::key(SDL_SCANCODE_P));
    set(Action::QuickSave, InputBinding::key(SDL_SCANCODE_F5));
    set(Action::DebugConsole, InputBinding::key(SDL_SCANCODE_GRAVE));
}

void ActionBindings::bind(Action a, int slot, InputBinding b) {
    if (slot < 0 || slot >= kSlots) {
        return;
    }
    // A physical input drives at most one action: unbind it elsewhere first.
    if (b.valid()) {
        for (auto& slots : m_bindings) {
            for (auto& existing : slots) {
                if (existing == b) {
                    existing = {};
                }
            }
        }
    }
    m_bindings[static_cast<usize>(a)][static_cast<usize>(slot)] = b;
}

const InputBinding& ActionBindings::get(Action a, int slot) const {
    return m_bindings[static_cast<usize>(a)][static_cast<usize>(slot)];
}

nlohmann::json ActionBindings::toJson() const {
    nlohmann::json j = nlohmann::json::object();
    for (usize i = 0; i < m_bindings.size(); ++i) {
        j[std::string(kActionNames[i])] = {m_bindings[i][0].toString(), m_bindings[i][1].toString()};
    }
    return j;
}

void ActionBindings::fromJson(const nlohmann::json& j) {
    if (!j.is_object()) {
        return;
    }
    for (auto it = j.begin(); it != j.end(); ++it) {
        const auto action = actionFromName(it.key());
        if (!action || !it.value().is_array()) {
            continue;
        }
        for (int s = 0; s < kSlots && s < static_cast<int>(it.value().size()); ++s) {
            if (it.value()[static_cast<usize>(s)].is_string()) {
                m_bindings[static_cast<usize>(*action)][static_cast<usize>(s)] =
                    InputBinding::fromString(it.value()[static_cast<usize>(s)].get<std::string>());
            }
        }
    }
}

void InputSystem::beginFrame() {
    m_keysPrev = m_keys;
    m_mousePrev = m_mouse;
    m_injectedPrev = m_injected;
    m_mouseDelta = {0.0f, 0.0f};
    m_wheel = 0.0f;
    m_text.clear();
    m_lastPressed.reset();
}

void InputSystem::processEvent(const SDL_Event& e) {
    switch (e.type) {
    case SDL_EVENT_KEY_DOWN:
        if (e.key.scancode < kMaxKeys) {
            if (!e.key.repeat) {
                m_lastPressed = InputBinding::key(e.key.scancode);
            }
            m_keys.set(e.key.scancode);
        }
        break;
    case SDL_EVENT_KEY_UP:
        if (e.key.scancode < kMaxKeys) {
            m_keys.reset(e.key.scancode);
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (e.button.button < kMaxMouseButtons) {
            m_mouse.set(e.button.button);
            m_lastPressed = InputBinding::mouse(e.button.button);
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (e.button.button < kMaxMouseButtons) {
            m_mouse.reset(e.button.button);
        }
        break;
    case SDL_EVENT_MOUSE_MOTION:
        m_mouseDelta += glm::vec2(e.motion.xrel, e.motion.yrel);
        m_mousePos = {e.motion.x, e.motion.y};
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        m_wheel += e.wheel.y;
        break;
    case SDL_EVENT_TEXT_INPUT:
        if (e.text.text) {
            m_text += e.text.text;
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        // Avoid "stuck" keys when alt-tabbing.
        m_keys.reset();
        m_mouse.reset();
        break;
    default:
        break;
    }
}

bool InputSystem::keyDown(int sc) const { return sc >= 0 && sc < kMaxKeys && m_keys.test(static_cast<usize>(sc)); }
bool InputSystem::keyPressed(int sc) const {
    return sc >= 0 && sc < kMaxKeys && m_keys.test(static_cast<usize>(sc)) && !m_keysPrev.test(static_cast<usize>(sc));
}
bool InputSystem::keyReleased(int sc) const {
    return sc >= 0 && sc < kMaxKeys && !m_keys.test(static_cast<usize>(sc)) && m_keysPrev.test(static_cast<usize>(sc));
}
bool InputSystem::mouseDown(int b) const { return b >= 0 && b < kMaxMouseButtons && m_mouse.test(static_cast<usize>(b)); }
bool InputSystem::mousePressed(int b) const {
    return b >= 0 && b < kMaxMouseButtons && m_mouse.test(static_cast<usize>(b)) && !m_mousePrev.test(static_cast<usize>(b));
}
bool InputSystem::mouseReleased(int b) const {
    return b >= 0 && b < kMaxMouseButtons && !m_mouse.test(static_cast<usize>(b)) && m_mousePrev.test(static_cast<usize>(b));
}

bool InputSystem::bindingDown(const InputBinding& b) const {
    switch (b.device) {
    case InputBinding::Device::Keyboard: return keyDown(b.code);
    case InputBinding::Device::Mouse: return mouseDown(b.code);
    default: return false;
    }
}
bool InputSystem::bindingPressed(const InputBinding& b) const {
    switch (b.device) {
    case InputBinding::Device::Keyboard: return keyPressed(b.code);
    case InputBinding::Device::Mouse: return mousePressed(b.code);
    default: return false;
    }
}
bool InputSystem::bindingReleased(const InputBinding& b) const {
    switch (b.device) {
    case InputBinding::Device::Keyboard: return keyReleased(b.code);
    case InputBinding::Device::Mouse: return mouseReleased(b.code);
    default: return false;
    }
}

bool InputSystem::actionDown(Action a) const {
    const usize i = static_cast<usize>(a);
    if (m_injected.test(i)) {
        return true;
    }
    return bindingDown(m_bindings.get(a, 0)) || bindingDown(m_bindings.get(a, 1));
}
bool InputSystem::actionPressed(Action a) const {
    const usize i = static_cast<usize>(a);
    if (m_injected.test(i) && !m_injectedPrev.test(i)) {
        return true;
    }
    return bindingPressed(m_bindings.get(a, 0)) || bindingPressed(m_bindings.get(a, 1));
}
bool InputSystem::actionReleased(Action a) const {
    const usize i = static_cast<usize>(a);
    if (!m_injected.test(i) && m_injectedPrev.test(i)) {
        return true;
    }
    return bindingReleased(m_bindings.get(a, 0)) || bindingReleased(m_bindings.get(a, 1));
}

void InputSystem::injectAction(Action a, bool down) { m_injected.set(static_cast<usize>(a), down); }

} // namespace br
