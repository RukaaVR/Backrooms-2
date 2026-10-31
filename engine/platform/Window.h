#pragma once

#include "engine/core/Types.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct SDL_Window;
union SDL_Event;

namespace br {

enum class FullscreenMode { Windowed, Borderless, Exclusive };

struct WindowConfig {
    std::string title = "THE BACKROOMS: BEYOND REALITY";
    int width = 1920;
    int height = 1080;
    FullscreenMode fullscreen = FullscreenMode::Windowed;
    bool vsync = true;
    bool hidden = false;       // used by tests / screenshot harness
    bool debugContext = false; // GL_KHR_debug output
    int glMajor = 4;
    int glMinor = 5;           // falls back to 4.3 automatically
};

struct DisplayResolution {
    int width = 0;
    int height = 0;
    float refreshRate = 0.0f;
};

// Owns the SDL subsystem lifetime. Exactly one instance per process.
class PlatformContext : NonCopyable {
public:
    PlatformContext();
    ~PlatformContext();
    bool ok() const { return m_ok; }

private:
    bool m_ok = false;
};

// SDL3 window with an OpenGL core-profile context. All GL calls must happen on
// the thread that created the window.
class Window : NonCopyable {
public:
    Window() = default;
    ~Window();

    bool create(const WindowConfig& config);
    void destroy();

    SDL_Window* handle() const { return m_window; }
    bool valid() const { return m_window != nullptr; }

    glm::ivec2 drawableSize() const; // pixels
    glm::ivec2 windowSize() const;   // logical units
    int glVersion() const { return m_glVersion; } // e.g. 45 for 4.5

    void swapBuffers();
    void setVsync(bool enabled);
    void setFullscreen(FullscreenMode mode, int width, int height);
    void setSize(int width, int height);
    void setTitle(const std::string& title);
    void setRelativeMouse(bool enabled);
    bool relativeMouse() const { return m_relativeMouse; }

    // Returns true when the event closed the window.
    bool handleEvent(const SDL_Event& e);
    bool closeRequested() const { return m_closeRequested; }
    void clearCloseRequest() { m_closeRequested = false; }
    bool wasResized() { const bool r = m_resized; m_resized = false; return r; }
    bool focused() const { return m_focused; }

    std::vector<DisplayResolution> availableResolutions() const;

private:
    SDL_Window* m_window = nullptr;
    void* m_glContext = nullptr;
    int m_glVersion = 0;
    bool m_closeRequested = false;
    bool m_resized = false;
    bool m_focused = true;
    bool m_relativeMouse = false;
};

} // namespace br
