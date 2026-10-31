#pragma once

#include "engine/core/Types.h"
#include "engine/renderer/GL.h"

#include <glm/glm.hpp>

#include <span>
#include <utility>
#include <vector>

namespace br {
struct Image;
}

namespace br::gpu {

// RAII wrappers around OpenGL 4.5 DSA objects. Each owns exactly one GL name
// and releases it on destruction; they are movable but not copyable.

class Buffer {
public:
    Buffer() = default;
    ~Buffer() { release(); }
    Buffer(Buffer&& o) noexcept : m_id(std::exchange(o.m_id, 0)), m_size(std::exchange(o.m_size, 0)) {}
    Buffer& operator=(Buffer&& o) noexcept {
        if (this != &o) {
            release();
            m_id = std::exchange(o.m_id, 0);
            m_size = std::exchange(o.m_size, 0);
        }
        return *this;
    }
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    // flags: GL_DYNAMIC_STORAGE_BIT etc. Immutable storage (glNamedBufferStorage).
    void create(usize size, const void* data, GLbitfield flags = GL_DYNAMIC_STORAGE_BIT);
    void update(usize offset, usize size, const void* data);
    // Grows (re-creates) if needed, then uploads. Contents are not preserved.
    void ensureAndUpload(usize size, const void* data);
    void release();

    GLuint id() const { return m_id; }
    usize size() const { return m_size; }
    bool valid() const { return m_id != 0; }

private:
    GLuint m_id = 0;
    usize m_size = 0;
};

enum class TextureFormat : u8 {
    RGBA8,
    SRGB8_A8,
    RG8,
    R8,
    RGBA16F,
    RG16F,
    R16F,
    R11G11B10F,
    RGBA32F,
    R32F,
    Depth24Stencil8,
    Depth32F,
};

GLenum toGLInternalFormat(TextureFormat f);

struct SamplerDesc {
    GLenum minFilter = GL_LINEAR_MIPMAP_LINEAR;
    GLenum magFilter = GL_LINEAR;
    GLenum wrapS = GL_REPEAT;
    GLenum wrapT = GL_REPEAT;
    GLenum wrapR = GL_REPEAT;
    float anisotropy = 1.0f;
    bool depthCompare = false; // sampler2DShadow
};

class Texture {
public:
    Texture() = default;
    ~Texture() { release(); }
    Texture(Texture&& o) noexcept { *this = std::move(o); }
    Texture& operator=(Texture&& o) noexcept {
        if (this != &o) {
            release();
            m_id = std::exchange(o.m_id, 0);
            m_target = o.m_target;
            m_width = o.m_width;
            m_height = o.m_height;
            m_depth = o.m_depth;
            m_levels = o.m_levels;
            m_format = o.m_format;
        }
        return *this;
    }
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void create2D(int w, int h, TextureFormat format, int levels = 1);
    void create2DArray(int w, int h, int layers, TextureFormat format, int levels = 1);
    void create3D(int w, int h, int d, TextureFormat format, int levels = 1);
    void createCube(int size, TextureFormat format, int levels = 1);
    // Uploads an Image (generating the full mip chain). srgb selects SRGB8_A8.
    void createFromImage(const Image& image, bool srgb, bool generateMips = true);

    void upload2D(int level, int x, int y, int w, int h, GLenum format, GLenum type, const void* data);
    void generateMips();
    void setSampler(const SamplerDesc& s);
    void bind(GLuint unit) const { glBindTextureUnit(unit, m_id); }
    void release();

    GLuint id() const { return m_id; }
    GLenum target() const { return m_target; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    int depth() const { return m_depth; }
    int levels() const { return m_levels; }
    TextureFormat format() const { return m_format; }
    bool valid() const { return m_id != 0; }

    static int mipCount(int w, int h);

private:
    GLuint m_id = 0;
    GLenum m_target = GL_TEXTURE_2D;
    int m_width = 0, m_height = 0, m_depth = 1, m_levels = 1;
    TextureFormat m_format = TextureFormat::RGBA8;
};

class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer() { release(); }
    Framebuffer(Framebuffer&& o) noexcept : m_id(std::exchange(o.m_id, 0)) {}
    Framebuffer& operator=(Framebuffer&& o) noexcept {
        if (this != &o) {
            release();
            m_id = std::exchange(o.m_id, 0);
        }
        return *this;
    }
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void create();
    void attachColor(int index, const Texture& tex, int level = 0);
    void attachColorLayer(int index, const Texture& tex, int layer, int level = 0);
    void attachDepth(const Texture& tex, int level = 0);
    void attachDepthLayer(const Texture& tex, int layer, int level = 0);
    void setDrawBuffers(std::span<const GLenum> buffers);
    bool complete() const;
    void bind() const { glBindFramebuffer(GL_FRAMEBUFFER, m_id); }
    void release();
    GLuint id() const { return m_id; }

private:
    GLuint m_id = 0;
};

class VertexArray {
public:
    VertexArray() = default;
    ~VertexArray() { release(); }
    VertexArray(VertexArray&& o) noexcept : m_id(std::exchange(o.m_id, 0)) {}
    VertexArray& operator=(VertexArray&& o) noexcept {
        if (this != &o) {
            release();
            m_id = std::exchange(o.m_id, 0);
        }
        return *this;
    }
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    void create();
    void release();
    void bind() const { glBindVertexArray(m_id); }
    GLuint id() const { return m_id; }

private:
    GLuint m_id = 0;
};

// Asynchronous GPU timer (GL_TIME_ELAPSED) with a small ring of queries so that
// reading results never stalls the pipeline.
class GpuTimer {
public:
    GpuTimer() = default;
    ~GpuTimer();
    GpuTimer(const GpuTimer&) = delete;
    GpuTimer& operator=(const GpuTimer&) = delete;
    void begin();
    void end();
    double lastMilliseconds() const { return m_lastMs; }

private:
    static constexpr int kRing = 4;
    GLuint m_queries[kRing] = {};
    bool m_pending[kRing] = {};
    int m_cursor = 0;
    bool m_created = false;
    double m_lastMs = 0.0;
};

} // namespace br::gpu
