#include "engine/renderer/Renderer.h"

#include "engine/core/Log.h"
#include "engine/core/Time.h"
#include "engine/resources/Image.h"
#include "engine/scene/MeshData.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace br::render {
namespace {

// std140 mirror of FrameData in engine/shaders/common.glsl.
struct FrameUniforms {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
    glm::mat4 invViewProj;
    glm::vec4 cameraPosTime;
    glm::vec4 ambient;
    glm::vec4 fogColorDensity;
    glm::vec4 fogParams; // x height falloff, y fog height, z light count, w exposure
    glm::vec4 sunDirIntensity;
    glm::vec4 sunColor;
    glm::vec4 skyZenith;
    glm::vec4 skyHorizon; // w = hasSky
};

// std430 mirror of GpuLight in engine/shaders/common.glsl.
struct GpuLight {
    glm::vec4 posRange;
    glm::vec4 colorIntensity;
    glm::vec4 dirType;
    glm::vec4 spotRect; // cos inner, cos outer, rect width, rect height
    glm::vec4 right;
};

constexpr usize kMaxForwardLights = 128;

} // namespace

glm::vec3 colorTemperatureToRgb(float kelvin) {
    const float t = std::clamp(kelvin, 1000.0f, 40000.0f) / 100.0f;
    float r, g, b;
    if (t <= 66.0f) {
        r = 255.0f;
        g = 99.4708025861f * std::log(t) - 161.1195681661f;
        b = t <= 19.0f ? 0.0f : 138.5177312231f * std::log(t - 10.0f) - 305.0447927307f;
    } else {
        r = 329.698727446f * std::pow(t - 60.0f, -0.1332047592f);
        g = 288.1221695283f * std::pow(t - 60.0f, -0.0755148492f);
        b = 255.0f;
    }
    glm::vec3 srgb = glm::clamp(glm::vec3(r, g, b) / 255.0f, 0.0f, 1.0f);
    // The fit produces display-referred sRGB; convert to linear.
    glm::vec3 lin;
    for (int i = 0; i < 3; ++i) {
        lin[i] = srgb[i] <= 0.04045f ? srgb[i] / 12.92f : std::pow((srgb[i] + 0.055f) / 1.055f, 2.4f);
    }
    const float maxc = std::max({lin.r, lin.g, lin.b, 1e-4f});
    return lin / maxc;
}

Renderer::Renderer() = default;
Renderer::~Renderer() { shutdown(); }

bool Renderer::initialize(const Paths& paths, glm::ivec2 outputSize, const RenderSettings& settings) {
    m_paths = paths;
    m_settings = settings;
    m_outputSize = glm::max(outputSize, glm::ivec2(1));
    m_shaders = std::make_unique<gpu::ShaderLibrary>(paths.shaderDir);

    createDefaultTextures();
    m_emptyVao.create();
    m_frameUbo.create(sizeof(FrameUniforms), nullptr);
    m_lightSsbo.create(sizeof(GpuLight) * kMaxForwardLights, nullptr);
    createTargets();

    // Compile everything up front so missing/broken shaders fail at startup.
    const bool ok = m_shaders->graphics("baseline/forward.vert", "baseline/forward.frag") &&
                    m_shaders->graphics("post/fullscreen.vert", "post/tonemap.frag");
    if (!ok) {
        BR_LOG_ERROR("renderer", "Failed to compile baseline shaders from '{}'", paths.shaderDir.string());
        return false;
    }
    BR_LOG_INFO("renderer", "Renderer initialised at {}x{} (scale {:.2f})", m_outputSize.x, m_outputSize.y,
                m_settings.renderScale);
    return true;
}

void Renderer::shutdown() {
    m_instances.clear();
    m_lights.clear();
    m_meshes.clear();
    m_textures.clear();
    m_materials.clear();
    m_shaders.reset();
}

void Renderer::createDefaultTextures() {
    auto solid = [](gpu::Texture& t, u8 r, u8 g, u8 b, u8 a, bool srgb) {
        Image img = Image::createU8(4, 4, 4);
        for (usize i = 0; i < img.pixels8.size(); i += 4) {
            img.pixels8[i] = r;
            img.pixels8[i + 1] = g;
            img.pixels8[i + 2] = b;
            img.pixels8[i + 3] = a;
        }
        t.createFromImage(img, srgb, false);
    };
    solid(m_whiteTex, 255, 255, 255, 255, true);
    solid(m_blackTex, 0, 0, 0, 255, true);
    solid(m_flatNormalTex, 128, 128, 255, 255, false);
    solid(m_defaultOrmTex, 255, 255, 0, 255, false);
}

void Renderer::createTargets() {
    m_internalSize = glm::max(glm::ivec2(glm::vec2(m_outputSize) * m_settings.renderScale), glm::ivec2(1));
    m_hdrColor.create2D(m_internalSize.x, m_internalSize.y, gpu::TextureFormat::RGBA16F, 1);
    m_depth.create2D(m_internalSize.x, m_internalSize.y, gpu::TextureFormat::Depth32F, 1);
    m_hdrFbo.create();
    m_hdrFbo.attachColor(0, m_hdrColor);
    m_hdrFbo.attachDepth(m_depth);
    const GLenum bufs[] = {GL_COLOR_ATTACHMENT0};
    m_hdrFbo.setDrawBuffers(bufs);
    m_hdrFbo.complete();
}

void Renderer::resize(glm::ivec2 outputSize) {
    outputSize = glm::max(outputSize, glm::ivec2(1));
    if (outputSize != m_outputSize) {
        m_outputSize = outputSize;
        createTargets();
    }
}

void Renderer::applySettings(const RenderSettings& settings) {
    const bool rescale = settings.renderScale != m_settings.renderScale;
    m_settings = settings;
    if (rescale) {
        createTargets();
    }
}

MeshHandle Renderer::createMesh(const MeshData& data) {
    GpuMesh m;
    m.vbo.create(data.vertices.size() * sizeof(Vertex), data.vertices.data(), 0);
    m.ibo.create(data.indices.size() * sizeof(u32), data.indices.data(), 0);
    m.indexCount = static_cast<u32>(data.indices.size());
    m.bounds = data.bounds();
    m.vao.create();
    const GLuint vao = m.vao.id();
    glVertexArrayVertexBuffer(vao, 0, m.vbo.id(), 0, sizeof(Vertex));
    glVertexArrayElementBuffer(vao, m.ibo.id());
    auto attrib = [&](GLuint loc, GLint count, GLenum type, GLboolean norm, usize offset) {
        glEnableVertexArrayAttrib(vao, loc);
        glVertexArrayAttribFormat(vao, loc, count, type, norm, static_cast<GLuint>(offset));
        glVertexArrayAttribBinding(vao, loc, 0);
    };
    attrib(0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    attrib(1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, normal));
    attrib(2, 4, GL_FLOAT, GL_FALSE, offsetof(Vertex, tangent));
    attrib(3, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, uv));
    attrib(4, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(Vertex, color));
    return m_meshes.insert(std::move(m));
}

void Renderer::destroyMesh(MeshHandle h) { m_meshes.erase(h); }

TextureHandle Renderer::createTexture(const Image& image, TextureUsage usage) {
    gpu::Texture t;
    t.createFromImage(image, usage == TextureUsage::ColorSRGB, true);
    gpu::SamplerDesc s;
    s.anisotropy = m_settings.anisotropy;
    t.setSampler(s);
    return m_textures.insert(std::move(t));
}

void Renderer::destroyTexture(TextureHandle h) { m_textures.erase(h); }

MaterialHandle Renderer::createMaterial(const MaterialDesc& desc) { return m_materials.insert(desc); }

void Renderer::updateMaterial(MaterialHandle h, const MaterialDesc& desc) {
    if (auto* m = m_materials.get(h)) {
        *m = desc;
    }
}

void Renderer::destroyMaterial(MaterialHandle h) { m_materials.erase(h); }
const MaterialDesc* Renderer::material(MaterialHandle h) const { return m_materials.get(h); }

InstanceHandle Renderer::createInstance(const InstanceDesc& desc) {
    Instance inst{desc, {}};
    if (const GpuMesh* m = m_meshes.get(desc.mesh)) {
        inst.worldBounds = m->bounds.transformed(desc.transform);
    }
    return m_instances.insert(inst);
}

void Renderer::setInstanceTransform(InstanceHandle h, const glm::mat4& t) {
    if (Instance* inst = m_instances.get(h)) {
        inst->desc.transform = t;
        if (const GpuMesh* m = m_meshes.get(inst->desc.mesh)) {
            inst->worldBounds = m->bounds.transformed(t);
        }
    }
}

void Renderer::setInstanceVisible(InstanceHandle h, bool visible) {
    if (Instance* inst = m_instances.get(h)) {
        inst->desc.visible = visible;
    }
}

void Renderer::setInstanceMaterial(InstanceHandle h, MaterialHandle material) {
    if (Instance* inst = m_instances.get(h)) {
        inst->desc.material = material;
    }
}

void Renderer::setInstanceParams(InstanceHandle h, const glm::vec4& params) {
    if (Instance* inst = m_instances.get(h)) {
        inst->desc.params = params;
    }
}

void Renderer::destroyInstance(InstanceHandle h) { m_instances.erase(h); }

LightHandle Renderer::createLight(const LightDesc& desc) { return m_lights.insert(desc); }
void Renderer::updateLight(LightHandle h, const LightDesc& desc) {
    if (auto* l = m_lights.get(h)) {
        *l = desc;
    }
}
const LightDesc* Renderer::light(LightHandle h) const { return m_lights.get(h); }
void Renderer::destroyLight(LightHandle h) { m_lights.erase(h); }

void Renderer::clearScene() {
    m_instances.clear();
    m_lights.clear();
}

const gpu::Texture* Renderer::resolveTexture(TextureHandle h, const gpu::Texture& fallback) const {
    const gpu::Texture* t = m_textures.get(h);
    return t ? t : &fallback;
}

void Renderer::renderFrame(const Camera& cameraIn, float dt) {
    Stopwatch cpu;
    m_time += dt;
    m_stats = {};
    m_gpuTimer.begin();

    Camera camera = cameraIn;
    camera.aspect = static_cast<float>(m_internalSize.x) / static_cast<float>(m_internalSize.y);
    const glm::mat4 view = camera.view();
    const glm::mat4 proj = camera.projection();
    const glm::mat4 viewProj = proj * view;
    const Frustum frustum = Frustum::fromMatrix(viewProj);

    // --- light selection: nearest visible lights up to the forward budget ---
    std::vector<std::pair<float, GpuLight>> candidates;
    m_lights.forEach([&](LightHandle, const LightDesc& l) {
        if (!l.enabled || l.intensity <= 0.0f) {
            return;
        }
        if (!frustum.intersectsSphere(l.position, l.range)) {
            return;
        }
        GpuLight g;
        g.posRange = glm::vec4(l.position, l.range);
        g.colorIntensity = glm::vec4(l.color, l.intensity);
        g.dirType = glm::vec4(glm::normalize(l.direction), static_cast<float>(l.type));
        g.spotRect = glm::vec4(std::cos(glm::radians(l.innerConeDegrees)), std::cos(glm::radians(l.outerConeDegrees)),
                               l.rectSize.x, l.rectSize.y);
        g.right = glm::vec4(glm::normalize(l.right), l.sourceRadius);
        candidates.emplace_back(glm::length(l.position - camera.position) - l.range, g);
    });
    std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<GpuLight> lights;
    for (usize i = 0; i < candidates.size() && i < kMaxForwardLights; ++i) {
        lights.push_back(candidates[i].second);
    }
    m_stats.visibleLights = static_cast<u32>(lights.size());
    if (!lights.empty()) {
        m_lightSsbo.update(0, lights.size() * sizeof(GpuLight), lights.data());
    }

    const EnvironmentDesc& env = m_environment;
    const float exposure = std::exp2(env.exposureCompensation);
    FrameUniforms fu;
    fu.view = view;
    fu.proj = proj;
    fu.viewProj = viewProj;
    fu.invViewProj = glm::inverse(viewProj);
    fu.cameraPosTime = glm::vec4(camera.position, m_time);
    fu.ambient = glm::vec4(env.ambientColor * env.ambientIntensity, 0.0f);
    fu.fogColorDensity = glm::vec4(env.fogColor, env.fogDensity);
    fu.fogParams = glm::vec4(env.fogHeightFalloff, env.fogHeight, static_cast<float>(lights.size()), exposure);
    fu.sunDirIntensity = glm::vec4(glm::normalize(env.sunDirection), env.sunIntensity);
    fu.sunColor = glm::vec4(env.sunColor, 0.0f);
    fu.skyZenith = glm::vec4(env.skyZenith, 0.0f);
    fu.skyHorizon = glm::vec4(env.skyHorizon, env.hasSky ? 1.0f : 0.0f);
    m_frameUbo.update(0, sizeof(fu), &fu);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_frameUbo.id());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_lightSsbo.id());

    // --- visibility + sort -------------------------------------------------
    struct Draw {
        u64 key;
        const Instance* inst;
    };
    std::vector<Draw> draws;
    m_instances.forEach([&](InstanceHandle, const Instance& inst) {
        ++m_stats.totalInstances;
        if (!inst.desc.visible || !frustum.intersects(inst.worldBounds)) {
            return;
        }
        draws.push_back({(static_cast<u64>(inst.desc.material.index) << 32) | inst.desc.mesh.index, &inst});
    });
    std::sort(draws.begin(), draws.end(), [](const Draw& a, const Draw& b) { return a.key < b.key; });
    m_stats.visibleInstances = static_cast<u32>(draws.size());

    // --- forward HDR pass ------------------------------------------------------
    m_hdrFbo.bind();
    glViewport(0, 0, m_internalSize.x, m_internalSize.y);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
    const glm::vec3 clear = env.hasSky ? env.skyHorizon : env.fogColor * (env.fogDensity > 0.0f ? 1.0f : 0.0f);
    glClearColor(clear.r, clear.g, clear.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto program = m_shaders->graphics("baseline/forward.vert", "baseline/forward.frag");
    if (program) {
        program->use();
        u32 lastMaterial = 0xFFFFFFFFu;
        for (const Draw& d : draws) {
            const GpuMesh* mesh = m_meshes.get(d.inst->desc.mesh);
            const MaterialDesc* mat = m_materials.get(d.inst->desc.material);
            if (!mesh || !mat || mat->blend == BlendMode::Transparent) {
                continue;
            }
            if (d.inst->desc.material.index != lastMaterial) {
                lastMaterial = d.inst->desc.material.index;
                resolveTexture(mat->albedo, m_whiteTex)->bind(0);
                resolveTexture(mat->normal, m_flatNormalTex)->bind(1);
                resolveTexture(mat->orm, m_defaultOrmTex)->bind(2);
                resolveTexture(mat->emissive, m_whiteTex)->bind(3);
                program->set("uBaseColor", mat->baseColor);
                program->set("uRoughness", mat->roughness);
                program->set("uMetallic", mat->metallic);
                program->set("uNormalStrength", mat->normalStrength);
                program->set("uOcclusionStrength", mat->occlusionStrength);
                program->set("uEmissive", mat->emissiveColor * mat->emissiveIntensity);
                program->set("uHasEmissiveTex", m_textures.contains(mat->emissive) ? 1 : 0);
                program->set("uUvScale", mat->uvScale);
                program->set("uAlphaCutoff", mat->blend == BlendMode::AlphaTest ? mat->alphaCutoff : -1.0f);
                if (mat->doubleSided) {
                    glDisable(GL_CULL_FACE);
                } else {
                    glEnable(GL_CULL_FACE);
                }
            }
            const glm::mat4& model = d.inst->desc.transform;
            program->set("uModel", model);
            program->set("uInstanceParams", d.inst->desc.params);
            program->set("uNormalMatrix", glm::mat3(glm::transpose(glm::inverse(glm::mat3(model)))));
            mesh->vao.bind();
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh->indexCount), GL_UNSIGNED_INT, nullptr);
            ++m_stats.drawCalls;
            m_stats.triangles += mesh->indexCount / 3;
        }
    }

    // --- tonemap to backbuffer --------------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_outputSize.x, m_outputSize.y);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    if (auto tonemap = m_shaders->graphics("post/fullscreen.vert", "post/tonemap.frag")) {
        tonemap->use();
        m_hdrColor.bind(0);
        tonemap->set("uExposure", exposure);
        tonemap->set("uBrightness", m_settings.brightness);
        tonemap->set("uSaturation", env.saturation);
        tonemap->set("uContrast", env.contrast);
        tonemap->set("uVignette", env.vignette);
        tonemap->set("uGrain", m_settings.filmGrain ? env.filmGrain : 0.0f);
        tonemap->set("uTime", m_time);
        m_emptyVao.bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glEnable(GL_DEPTH_TEST);

    m_gpuTimer.end();
    m_stats.gpuMilliseconds = m_gpuTimer.lastMilliseconds();
    m_stats.cpuMilliseconds = cpu.elapsedMilliseconds();
}

std::optional<Image> Renderer::captureFrame() const {
    Image img = Image::createU8(m_outputSize.x, m_outputSize.y, 4);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, m_outputSize.x, m_outputSize.y, GL_RGBA, GL_UNSIGNED_BYTE, img.pixels8.data());
    // GL origin is bottom-left; flip to top-left for PNG.
    const usize row = static_cast<usize>(m_outputSize.x) * 4;
    std::vector<u8> tmp(row);
    for (int y = 0; y < m_outputSize.y / 2; ++y) {
        u8* a = img.pixels8.data() + static_cast<usize>(y) * row;
        u8* b = img.pixels8.data() + static_cast<usize>(m_outputSize.y - 1 - y) * row;
        std::copy(a, a + row, tmp.data());
        std::copy(b, b + row, a);
        std::copy(tmp.data(), tmp.data() + row, b);
    }
    for (usize i = 3; i < img.pixels8.size(); i += 4) {
        img.pixels8[i] = 255;
    }
    return img;
}

} // namespace br::render
