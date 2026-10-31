#include "engine/renderer/GpuResources.h"

#include "engine/core/Log.h"
#include "engine/resources/Image.h"

#include <algorithm>
#include <cmath>

namespace br::gpu {

// ---------------------------------------------------------------- Buffer ----

void Buffer::create(usize size, const void* data, GLbitfield flags) {
    release();
    glCreateBuffers(1, &m_id);
    glNamedBufferStorage(m_id, static_cast<GLsizeiptr>(std::max<usize>(size, 16)), data, flags);
    m_size = size;
}

void Buffer::update(usize offset, usize size, const void* data) {
    if (size > 0) {
        glNamedBufferSubData(m_id, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
    }
}

void Buffer::ensureAndUpload(usize size, const void* data) {
    if (!valid() || size > m_size) {
        // Grow geometrically to avoid re-allocating every frame.
        create(std::max<usize>(size, m_size + m_size / 2), nullptr, GL_DYNAMIC_STORAGE_BIT);
    }
    update(0, size, data);
}

void Buffer::release() {
    if (m_id) {
        glDeleteBuffers(1, &m_id);
        m_id = 0;
        m_size = 0;
    }
}

// --------------------------------------------------------------- Texture ----

GLenum toGLInternalFormat(TextureFormat f) {
    switch (f) {
    case TextureFormat::RGBA8: return GL_RGBA8;
    case TextureFormat::SRGB8_A8: return GL_SRGB8_ALPHA8;
    case TextureFormat::RG8: return GL_RG8;
    case TextureFormat::R8: return GL_R8;
    case TextureFormat::RGBA16F: return GL_RGBA16F;
    case TextureFormat::RG16F: return GL_RG16F;
    case TextureFormat::R16F: return GL_R16F;
    case TextureFormat::R11G11B10F: return GL_R11F_G11F_B10F;
    case TextureFormat::RGBA32F: return GL_RGBA32F;
    case TextureFormat::R32F: return GL_R32F;
    case TextureFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
    case TextureFormat::Depth32F: return GL_DEPTH_COMPONENT32F;
    }
    return GL_RGBA8;
}

int Texture::mipCount(int w, int h) {
    return 1 + static_cast<int>(std::floor(std::log2(static_cast<float>(std::max(1, std::max(w, h))))));
}

void Texture::create2D(int w, int h, TextureFormat format, int levels) {
    release();
    m_target = GL_TEXTURE_2D;
    glCreateTextures(m_target, 1, &m_id);
    m_width = w;
    m_height = h;
    m_depth = 1;
    m_levels = levels <= 0 ? mipCount(w, h) : levels;
    m_format = format;
    glTextureStorage2D(m_id, m_levels, toGLInternalFormat(format), w, h);
    SamplerDesc s;
    s.minFilter = m_levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    s.wrapS = s.wrapT = s.wrapR = GL_CLAMP_TO_EDGE;
    setSampler(s);
}

void Texture::create2DArray(int w, int h, int layers, TextureFormat format, int levels) {
    release();
    m_target = GL_TEXTURE_2D_ARRAY;
    glCreateTextures(m_target, 1, &m_id);
    m_width = w;
    m_height = h;
    m_depth = layers;
    m_levels = levels <= 0 ? mipCount(w, h) : levels;
    m_format = format;
    glTextureStorage3D(m_id, m_levels, toGLInternalFormat(format), w, h, layers);
    SamplerDesc s;
    s.minFilter = m_levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    s.wrapS = s.wrapT = s.wrapR = GL_CLAMP_TO_EDGE;
    setSampler(s);
}

void Texture::create3D(int w, int h, int d, TextureFormat format, int levels) {
    release();
    m_target = GL_TEXTURE_3D;
    glCreateTextures(m_target, 1, &m_id);
    m_width = w;
    m_height = h;
    m_depth = d;
    m_levels = std::max(1, levels);
    m_format = format;
    glTextureStorage3D(m_id, m_levels, toGLInternalFormat(format), w, h, d);
    SamplerDesc s;
    s.minFilter = GL_LINEAR;
    s.wrapS = s.wrapT = s.wrapR = GL_CLAMP_TO_EDGE;
    setSampler(s);
}

void Texture::createCube(int size, TextureFormat format, int levels) {
    release();
    m_target = GL_TEXTURE_CUBE_MAP;
    glCreateTextures(m_target, 1, &m_id);
    m_width = m_height = size;
    m_depth = 6;
    m_levels = levels <= 0 ? mipCount(size, size) : levels;
    m_format = format;
    glTextureStorage2D(m_id, m_levels, toGLInternalFormat(format), size, size);
    SamplerDesc s;
    s.minFilter = m_levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    s.wrapS = s.wrapT = s.wrapR = GL_CLAMP_TO_EDGE;
    setSampler(s);
}

void Texture::createFromImage(const Image& image, bool srgb, bool generateMipChain) {
    const int levels = generateMipChain ? mipCount(image.width, image.height) : 1;
    TextureFormat fmt;
    GLenum uploadFormat;
    if (image.format == Image::Format::F32) {
        fmt = image.channels == 1 ? TextureFormat::R16F : (image.channels == 2 ? TextureFormat::RG16F : TextureFormat::RGBA16F);
    } else if (image.channels == 4) {
        fmt = srgb ? TextureFormat::SRGB8_A8 : TextureFormat::RGBA8;
    } else if (image.channels == 2) {
        fmt = TextureFormat::RG8;
    } else if (image.channels == 1) {
        fmt = TextureFormat::R8;
    } else {
        fmt = srgb ? TextureFormat::SRGB8_A8 : TextureFormat::RGBA8;
    }
    switch (image.channels) {
    case 1: uploadFormat = GL_RED; break;
    case 2: uploadFormat = GL_RG; break;
    case 3: uploadFormat = GL_RGB; break;
    default: uploadFormat = GL_RGBA; break;
    }
    create2D(image.width, image.height, fmt, levels);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (image.format == Image::Format::F32) {
        upload2D(0, 0, 0, image.width, image.height, uploadFormat, GL_FLOAT, image.pixelsF.data());
    } else {
        upload2D(0, 0, 0, image.width, image.height, uploadFormat, GL_UNSIGNED_BYTE, image.pixels8.data());
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    if (levels > 1) {
        generateMips();
    }
    SamplerDesc s;
    s.minFilter = levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    s.wrapS = s.wrapT = GL_REPEAT;
    s.anisotropy = 8.0f;
    setSampler(s);
}

void Texture::upload2D(int level, int x, int y, int w, int h, GLenum format, GLenum type, const void* data) {
    glTextureSubImage2D(m_id, level, x, y, w, h, format, type, data);
}

void Texture::generateMips() { glGenerateTextureMipmap(m_id); }

void Texture::setSampler(const SamplerDesc& s) {
    glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(s.minFilter));
    glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(s.magFilter));
    glTextureParameteri(m_id, GL_TEXTURE_WRAP_S, static_cast<GLint>(s.wrapS));
    glTextureParameteri(m_id, GL_TEXTURE_WRAP_T, static_cast<GLint>(s.wrapT));
    glTextureParameteri(m_id, GL_TEXTURE_WRAP_R, static_cast<GLint>(s.wrapR));
    if (s.anisotropy > 1.0f && (GLAD_GL_ARB_texture_filter_anisotropic || GLAD_GL_EXT_texture_filter_anisotropic)) {
        GLfloat maxAniso = 1.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
        glTextureParameterf(m_id, GL_TEXTURE_MAX_ANISOTROPY, std::min(s.anisotropy, maxAniso));
    }
    if (s.depthCompare) {
        glTextureParameteri(m_id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTextureParameteri(m_id, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    }
}

void Texture::release() {
    if (m_id) {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}

// ----------------------------------------------------------- Framebuffer ----

void Framebuffer::create() {
    release();
    glCreateFramebuffers(1, &m_id);
}

void Framebuffer::attachColor(int index, const Texture& tex, int level) {
    glNamedFramebufferTexture(m_id, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(index), tex.id(), level);
}

void Framebuffer::attachColorLayer(int index, const Texture& tex, int layer, int level) {
    glNamedFramebufferTextureLayer(m_id, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(index), tex.id(), level, layer);
}

void Framebuffer::attachDepth(const Texture& tex, int level) {
    const GLenum attachment =
        tex.format() == TextureFormat::Depth24Stencil8 ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
    glNamedFramebufferTexture(m_id, attachment, tex.id(), level);
}

void Framebuffer::attachDepthLayer(const Texture& tex, int layer, int level) {
    glNamedFramebufferTextureLayer(m_id, GL_DEPTH_ATTACHMENT, tex.id(), level, layer);
}

void Framebuffer::setDrawBuffers(std::span<const GLenum> buffers) {
    if (buffers.empty()) {
        glNamedFramebufferDrawBuffer(m_id, GL_NONE);
        glNamedFramebufferReadBuffer(m_id, GL_NONE);
    } else {
        glNamedFramebufferDrawBuffers(m_id, static_cast<GLsizei>(buffers.size()), buffers.data());
    }
}

bool Framebuffer::complete() const {
    const GLenum status = glCheckNamedFramebufferStatus(m_id, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        BR_LOG_ERROR("gpu", "Framebuffer {} incomplete: 0x{:x}", m_id, status);
        return false;
    }
    return true;
}

void Framebuffer::release() {
    if (m_id) {
        glDeleteFramebuffers(1, &m_id);
        m_id = 0;
    }
}

// ----------------------------------------------------------- VertexArray ----

void VertexArray::create() {
    release();
    glCreateVertexArrays(1, &m_id);
}

void VertexArray::release() {
    if (m_id) {
        glDeleteVertexArrays(1, &m_id);
        m_id = 0;
    }
}

// -------------------------------------------------------------- GpuTimer ----

GpuTimer::~GpuTimer() {
    if (m_created) {
        glDeleteQueries(kRing, m_queries);
    }
}

void GpuTimer::begin() {
    if (!m_created) {
        glGenQueries(kRing, m_queries);
        m_created = true;
    }
    // Harvest the oldest query if its result is ready.
    const int slot = m_cursor;
    if (m_pending[slot]) {
        GLint available = 0;
        glGetQueryObjectiv(m_queries[slot], GL_QUERY_RESULT_AVAILABLE, &available);
        if (available) {
            GLuint64 ns = 0;
            glGetQueryObjectui64v(m_queries[slot], GL_QUERY_RESULT, &ns);
            m_lastMs = static_cast<double>(ns) * 1e-6;
        }
        m_pending[slot] = false;
    }
    glBeginQuery(GL_TIME_ELAPSED, m_queries[slot]);
}

void GpuTimer::end() {
    glEndQuery(GL_TIME_ELAPSED);
    m_pending[m_cursor] = true;
    m_cursor = (m_cursor + 1) % kRing;
}

} // namespace br::gpu
