#include "engine/platform/Window.h"

#include "engine/core/Log.h"
#include "engine/renderer/GL.h"

#include <SDL3/SDL.h>

#include <algorithm>

namespace br {

PlatformContext::PlatformContext() {
    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0");
    m_ok = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if (!m_ok) {
        BR_LOG_ERROR("platform", "SDL_Init failed: {}", SDL_GetError());
    } else {
        BR_LOG_INFO("platform", "SDL {}.{}.{} initialised (video driver: {})", SDL_MAJOR_VERSION, SDL_MINOR_VERSION,
                    SDL_MICRO_VERSION, SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "none");
    }
}

PlatformContext::~PlatformContext() {
    if (m_ok) {
        SDL_Quit();
    }
}

Window::~Window() { destroy(); }

namespace {

void GLAD_API_PTR glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei, const GLchar* message,
                              const void*) {
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }
    if (type == GL_DEBUG_TYPE_ERROR) {
        BR_LOG_ERROR("gl", "GL error {}: {}", id, message);
    } else if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM) {
        BR_LOG_WARN("gl", "GL {}: {}", id, message);
    }
    (void)source;
}

} // namespace

bool Window::create(const WindowConfig& config) {
    destroy();

    SDL_WindowFlags flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (config.hidden) {
        flags |= SDL_WINDOW_HIDDEN;
    }

    const int versions[][2] = {{config.glMajor, config.glMinor}, {4, 5}, {4, 3}};
    for (const auto& v : versions) {
        SDL_GL_ResetAttributes();
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, v[0]);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, v[1]);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        // The default framebuffer only receives the final tonemapped image,
        // which the renderer encodes to sRGB itself.
        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 0);
        if (config.debugContext) {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
        }

        m_window = SDL_CreateWindow(config.title.c_str(), config.width, config.height, flags);
        if (!m_window) {
            BR_LOG_ERROR("platform", "SDL_CreateWindow failed: {}", SDL_GetError());
            return false;
        }
        m_glContext = SDL_GL_CreateContext(m_window);
        if (m_glContext) {
            break;
        }
        BR_LOG_WARN("platform", "OpenGL {}.{} context unavailable: {}", v[0], v[1], SDL_GetError());
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    if (!m_glContext) {
        BR_LOG_ERROR("platform", "No suitable OpenGL 4.3+ core context");
        return false;
    }
    SDL_GL_MakeCurrent(m_window, static_cast<SDL_GLContext>(m_glContext));

    const int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
    if (version == 0) {
        BR_LOG_ERROR("platform", "gladLoadGL failed");
        destroy();
        return false;
    }
    m_glVersion = GLAD_VERSION_MAJOR(version) * 10 + GLAD_VERSION_MINOR(version);
    if (m_glVersion < 43) {
        BR_LOG_ERROR("platform", "OpenGL 4.3 required, got {}.{}", m_glVersion / 10, m_glVersion % 10);
        destroy();
        return false;
    }
    BR_LOG_INFO("platform", "OpenGL {} | {} | {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)),
                reinterpret_cast<const char*>(glGetString(GL_RENDERER)),
                reinterpret_cast<const char*>(glGetString(GL_VENDOR)));

    if (config.debugContext && GLAD_GL_KHR_debug) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugCallback, nullptr);
    }

    setVsync(config.vsync);
    if (config.fullscreen != FullscreenMode::Windowed) {
        setFullscreen(config.fullscreen, config.width, config.height);
    }
    return true;
}

void Window::destroy() {
    if (m_glContext) {
        SDL_GL_DestroyContext(static_cast<SDL_GLContext>(m_glContext));
        m_glContext = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
}

glm::ivec2 Window::drawableSize() const {
    int w = 0, h = 0;
    if (m_window) {
        SDL_GetWindowSizeInPixels(m_window, &w, &h);
    }
    return {std::max(1, w), std::max(1, h)};
}

glm::ivec2 Window::windowSize() const {
    int w = 0, h = 0;
    if (m_window) {
        SDL_GetWindowSize(m_window, &w, &h);
    }
    return {std::max(1, w), std::max(1, h)};
}

void Window::swapBuffers() {
    if (m_window) {
        SDL_GL_SwapWindow(m_window);
    }
}

void Window::setVsync(bool enabled) {
    // Prefer adaptive vsync; fall back to regular vsync where unsupported.
    if (enabled) {
        if (!SDL_GL_SetSwapInterval(-1)) {
            SDL_GL_SetSwapInterval(1);
        }
    } else {
        SDL_GL_SetSwapInterval(0);
    }
}

void Window::setFullscreen(FullscreenMode mode, int width, int height) {
    if (!m_window) {
        return;
    }
    if (mode == FullscreenMode::Windowed) {
        SDL_SetWindowFullscreen(m_window, false);
        SDL_SetWindowSize(m_window, width, height);
        SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        return;
    }
    if (mode == FullscreenMode::Exclusive) {
        SDL_DisplayMode closest{};
        const SDL_DisplayID display = SDL_GetDisplayForWindow(m_window);
        if (SDL_GetClosestFullscreenDisplayMode(display, width, height, 0.0f, true, &closest)) {
            SDL_SetWindowFullscreenMode(m_window, &closest);
        } else {
            SDL_SetWindowFullscreenMode(m_window, nullptr);
        }
    } else {
        SDL_SetWindowFullscreenMode(m_window, nullptr); // borderless desktop
    }
    SDL_SetWindowFullscreen(m_window, true);
    m_resized = true;
}

void Window::setSize(int width, int height) {
    if (m_window) {
        SDL_SetWindowSize(m_window, width, height);
        m_resized = true;
    }
}

void Window::setTitle(const std::string& title) {
    if (m_window) {
        SDL_SetWindowTitle(m_window, title.c_str());
    }
}

void Window::setRelativeMouse(bool enabled) {
    if (m_window && enabled != m_relativeMouse) {
        SDL_SetWindowRelativeMouseMode(m_window, enabled);
        m_relativeMouse = enabled;
    }
}

bool Window::handleEvent(const SDL_Event& e) {
    switch (e.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        m_closeRequested = true;
        return true;
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_RESIZED:
        m_resized = true;
        break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        m_focused = true;
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        m_focused = false;
        break;
    default:
        break;
    }
    return false;
}

std::vector<DisplayResolution> Window::availableResolutions() const {
    std::vector<DisplayResolution> out;
    const SDL_DisplayID display = m_window ? SDL_GetDisplayForWindow(m_window) : SDL_GetPrimaryDisplay();
    int count = 0;
    SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(display, &count);
    if (modes) {
        for (int i = 0; i < count; ++i) {
            const DisplayResolution r{modes[i]->w, modes[i]->h, modes[i]->refresh_rate};
            const bool dup = std::any_of(out.begin(), out.end(),
                                         [&](const DisplayResolution& o) { return o.width == r.width && o.height == r.height; });
            if (!dup) {
                out.push_back(r);
            }
        }
        SDL_free(modes);
    }
    if (out.empty()) {
        for (auto [w, h] : {std::pair{1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3840, 2160}}) {
            out.push_back({w, h, 60.0f});
        }
    }
    std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        return a.width * a.height < b.width * b.height;
    });
    return out;
}

} // namespace br
