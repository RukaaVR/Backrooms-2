#pragma once

#include "engine/core/FileSystem.h"
#include "engine/core/Handle.h"
#include "engine/renderer/Camera.h"
#include "engine/renderer/GpuResources.h"
#include "engine/renderer/RenderTypes.h"
#include "engine/renderer/Shader.h"

#include <memory>
#include <optional>
#include <vector>

namespace br {
struct Image;
struct MeshData;
} // namespace br

namespace br::render {

// Retained-mode scene renderer. Game code creates meshes, materials, instances
// and lights once (e.g. when a chunk streams in) and only updates what moves.
// The renderer owns all GPU resources and is the only module issuing GL draw
// calls for the 3D world. Must be used from the GL thread.
class Renderer {
public:
    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool initialize(const Paths& paths, glm::ivec2 outputSize, const RenderSettings& settings);
    void shutdown();

    void resize(glm::ivec2 outputSize);
    void applySettings(const RenderSettings& settings);
    const RenderSettings& settings() const { return m_settings; }

    // --- resources -----------------------------------------------------------
    MeshHandle createMesh(const MeshData& data);
    void destroyMesh(MeshHandle h);
    TextureHandle createTexture(const Image& image, TextureUsage usage);
    void destroyTexture(TextureHandle h);
    MaterialHandle createMaterial(const MaterialDesc& desc);
    void updateMaterial(MaterialHandle h, const MaterialDesc& desc);
    void destroyMaterial(MaterialHandle h);
    const MaterialDesc* material(MaterialHandle h) const;

    // --- scene ---------------------------------------------------------------
    InstanceHandle createInstance(const InstanceDesc& desc);
    void setInstanceTransform(InstanceHandle h, const glm::mat4& transform);
    void setInstanceVisible(InstanceHandle h, bool visible);
    void setInstanceMaterial(InstanceHandle h, MaterialHandle material);
    void setInstanceParams(InstanceHandle h, const glm::vec4& params);
    void destroyInstance(InstanceHandle h);

    LightHandle createLight(const LightDesc& desc);
    void updateLight(LightHandle h, const LightDesc& desc);
    const LightDesc* light(LightHandle h) const;
    void destroyLight(LightHandle h);

    void setEnvironment(const EnvironmentDesc& env) { m_environment = env; }
    const EnvironmentDesc& environment() const { return m_environment; }

    // Destroys every instance and light (level change). Meshes/materials stay.
    void clearScene();

    // --- frame ---------------------------------------------------------------
    // Renders the scene to the default framebuffer (tonemapped, sRGB encoded).
    // UI is drawn by the caller afterwards on the same framebuffer.
    void renderFrame(const Camera& camera, float deltaSeconds);

    // Reads back the last presented frame (call after renderFrame, before swap).
    std::optional<Image> captureFrame() const;

    const FrameStats& stats() const { return m_stats; }
    glm::ivec2 outputSize() const { return m_outputSize; }
    gpu::ShaderLibrary& shaders() { return *m_shaders; }

private:
    struct GpuMesh {
        gpu::VertexArray vao;
        gpu::Buffer vbo;
        gpu::Buffer ibo;
        u32 indexCount = 0;
        AABB bounds;
    };
    struct Instance {
        InstanceDesc desc;
        AABB worldBounds;
    };

    void createTargets();
    void createDefaultTextures();
    const gpu::Texture* resolveTexture(TextureHandle h, const gpu::Texture& fallback) const;

    Paths m_paths;
    RenderSettings m_settings;
    EnvironmentDesc m_environment;
    glm::ivec2 m_outputSize{1, 1};
    glm::ivec2 m_internalSize{1, 1};
    FrameStats m_stats;
    float m_time = 0.0f;
    float m_exposureEV = 0.0f;

    std::unique_ptr<gpu::ShaderLibrary> m_shaders;
    SlotMap<MeshTag, GpuMesh> m_meshes;
    SlotMap<TextureTag, gpu::Texture> m_textures;
    SlotMap<MaterialTag, MaterialDesc> m_materials;
    SlotMap<InstanceTag, Instance> m_instances;
    SlotMap<LightTag, LightDesc> m_lights;

    gpu::Texture m_whiteTex, m_flatNormalTex, m_defaultOrmTex, m_blackTex;
    gpu::Texture m_hdrColor, m_depth;
    gpu::Framebuffer m_hdrFbo;
    gpu::VertexArray m_emptyVao;
    gpu::Buffer m_frameUbo, m_lightSsbo;
    gpu::GpuTimer m_gpuTimer;
    std::vector<u32> m_drawOrder;
};

} // namespace br::render
