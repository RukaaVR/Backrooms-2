#pragma once

#include "engine/core/Types.h"

#include <glm/glm.hpp>
#include <nlohmann/json_fwd.hpp>

#include <array>
#include <bitset>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

union SDL_Event;

namespace br {

// Logical game actions. Gameplay code never queries raw keys; it queries
// actions so every control is rebindable from the settings menu.
enum class Action : u8 {
    MoveForward,
    MoveBackward,
    MoveLeft,
    MoveRight,
    Jump,
    Crouch,
    Sprint,
    Interact,   // use / pick up / open
    Throw,      // throw held object
    Flashlight,
    Inventory,
    UseItem,
    Pause,
    QuickSave,
    DebugConsole,
    Count
};

std::string_view actionName(Action a);
std::optional<Action> actionFromName(std::string_view name);

struct InputBinding {
    enum class Device : u8 { None, Keyboard, Mouse };
    Device device = Device::None;
    int code = 0; // SDL_Scancode for keyboard, SDL mouse button index for mouse

    static InputBinding key(int scancode) { return {Device::Keyboard, scancode}; }
    static InputBinding mouse(int button) { return {Device::Mouse, button}; }
    bool valid() const { return device != Device::None; }
    std::string toString() const;
    static InputBinding fromString(std::string_view s);
    friend bool operator==(const InputBinding&, const InputBinding&) = default;
};

// Two bindings (primary/secondary) per action.
class ActionBindings {
public:
    static constexpr int kSlots = 2;
    ActionBindings(); // installs defaults

    void resetToDefaults();
    void bind(Action a, int slot, InputBinding b);
    const InputBinding& get(Action a, int slot) const;

    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);

private:
    std::array<std::array<InputBinding, kSlots>, static_cast<usize>(Action::Count)> m_bindings{};
};

class InputSystem {
public:
    static constexpr int kMaxKeys = 512;
    static constexpr int kMaxMouseButtons = 8;

    void beginFrame();
    void processEvent(const SDL_Event& e);

    bool keyDown(int scancode) const;
    bool keyPressed(int scancode) const;
    bool keyReleased(int scancode) const;
    bool mouseDown(int button) const;
    bool mousePressed(int button) const;
    bool mouseReleased(int button) const;

    bool actionDown(Action a) const;
    bool actionPressed(Action a) const;
    bool actionReleased(Action a) const;

    glm::vec2 mouseDelta() const { return m_mouseDelta; }
    glm::vec2 mousePosition() const { return m_mousePos; } // window coordinates
    float wheelDelta() const { return m_wheel; }
    const std::string& textInput() const { return m_text; }

    // The most recent key/button pressed this frame (for keybinding capture).
    std::optional<InputBinding> lastPressed() const { return m_lastPressed; }

    ActionBindings& bindings() { return m_bindings; }
    const ActionBindings& bindings() const { return m_bindings; }

    // Suppresses gameplay input (e.g. while a menu is open) without losing state.
    void setGameplayBlocked(bool blocked) { m_blocked = blocked; }
    bool gameplayBlocked() const { return m_blocked; }

    // Test/automation hook: inject an action state for the current frame.
    void injectAction(Action a, bool down);
    void injectMouseDelta(glm::vec2 d) { m_mouseDelta += d; }

private:
    bool bindingDown(const InputBinding& b) const;
    bool bindingPressed(const InputBinding& b) const;
    bool bindingReleased(const InputBinding& b) const;

    std::bitset<kMaxKeys> m_keys, m_keysPrev;
    std::bitset<kMaxMouseButtons> m_mouse, m_mousePrev;
    std::bitset<static_cast<usize>(Action::Count)> m_injected, m_injectedPrev;
    glm::vec2 m_mouseDelta{0.0f};
    glm::vec2 m_mousePos{0.0f};
    float m_wheel = 0.0f;
    std::string m_text;
    std::optional<InputBinding> m_lastPressed;
    ActionBindings m_bindings;
    bool m_blocked = false;
};

} // namespace br
