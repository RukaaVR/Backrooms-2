#pragma once

#include "engine/core/Handle.h"
#include "engine/core/Math.h"
#include "engine/core/Types.h"

#include <string>

namespace br::render {

struct MeshTag {};
struct MaterialTag {};
struct TextureTag {};
struct InstanceTag {};
struct LightTag {};

using MeshHandle = Handle<MeshTag>;
using MaterialHandle = Handle<MaterialTag>;
using TextureHandle = Handle<TextureTag>;
using InstanceHandle = Handle<InstanceTag>;
using LightHandle = Handle<LightTag>;

// How texel data is interpreted. Albedo/emissive are authored in sRGB and
// decoded by the hardware; everything else is linear data.
enum class TextureUsage : u8 { ColorSRGB, Linear, Normal };

enum class BlendMode : u8 { Opaque, AlphaTest, Transparent };

// Metallic-roughness PBR material (glTF 2.0 conventions).
//   orm texture: R = ambient occlusion, G = roughness, B = metallic
//   Factors multiply texture values; missing textures behave as white/flat.
struct MaterialDesc {
    std::string name;
    TextureHandle albedo;
    TextureHandle normal;
    TextureHandle orm;
    TextureHandle emissive;
    glm::vec4 baseColor{1.0f};       // linear RGB + alpha
    float roughness = 1.0f;
    float metallic = 0.0f;
    float normalStrength = 1.0f;
    float occlusionStrength = 1.0f;
    glm::vec3 emissiveColor{0.0f};   // linear
    float emissiveIntensity = 0.0f;  // HDR multiplier (nits-like scale)
    glm::vec2 uvScale{1.0f};
    BlendMode blend = BlendMode::Opaque;
    float alphaCutoff = 0.5f;
    bool doubleSided = false;
    float transmission = 0.0f;       // glass/water style transparency (forward pass)
    float ior = 1.5f;
};

enum class LightType : u8 { Point, Spot, Rect };

// Light intensity is expressed in an engine-relative photometric scale: a
// value of 1.0 approximates a 100 lm source. Colour is linear and normalised;
// use colorTemperatureToRgb() to derive it from Kelvin.
struct LightDesc {
    LightType type = LightType::Point;
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f, -1.0f, 0.0f}; // spot/rect emission direction
    glm::vec3 right{1.0f, 0.0f, 0.0f};      // rect light tangent
    glm::vec2 rectSize{0.6f, 1.2f};         // rect light extents in metres
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    float innerConeDegrees = 30.0f;
    float outerConeDegrees = 45.0f;
    float sourceRadius = 0.05f;             // soft shadow penumbra / specular size
    bool castShadows = false;
    bool enabled = true;
    float volumetricScale = 0.0f;           // contribution to volumetric fog
    float specularScale = 1.0f;
    float indirectScale = 1.0f;             // weight for bounce-light approximation
};

struct InstanceDesc {
    MeshHandle mesh;
    MaterialHandle material;
    glm::mat4 transform{1.0f};
    bool castShadows = true;
    bool visible = true;
    bool isStatic = true; // static instances may be baked / cached by the renderer
    u32 userTag = 0;
    // Per-instance shader parameters: x = emissive multiplier (fixture flicker),
    // y = wetness/dirt blend, z/w reserved.
    glm::vec4 params{1.0f, 0.0f, 0.0f, 0.0f};
};

// Per-level look. All colours linear.
struct EnvironmentDesc {
    glm::vec3 ambientColor{0.02f};      // fallback ambient irradiance
    float ambientIntensity = 1.0f;
    glm::vec3 fogColor{0.5f};
    float fogDensity = 0.0f;            // exponential fog, per metre
    float fogHeightFalloff = 0.0f;      // 0 = homogeneous
    float fogHeight = 0.0f;
    float volumetricStrength = 0.0f;    // in-scattering from lights (light shafts)
    bool hasSky = false;
    glm::vec3 skyZenith{0.02f, 0.03f, 0.06f};
    glm::vec3 skyHorizon{0.1f, 0.1f, 0.12f};
    glm::vec3 sunDirection{0.3f, -0.8f, 0.2f}; // direction light travels
    glm::vec3 sunColor{1.0f};
    float sunIntensity = 0.0f;
    bool sunShadows = true;
    // Camera / grading
    float exposureCompensation = 0.0f;  // EV
    float minExposureEV = -4.0f;
    float maxExposureEV = 8.0f;
    float adaptationSpeed = 1.5f;
    float bloomIntensity = 0.04f;
    float bloomThreshold = 1.0f;
    float saturation = 1.0f;
    float contrast = 1.0f;
    glm::vec3 colorFilter{1.0f};        // white balance / tint multiplier
    glm::vec3 lift{0.0f}, gamma{1.0f}, gain{1.0f};
    float vignette = 0.25f;
    float filmGrain = 0.03f;
    float chromaticAberration = 0.0f;
};

enum class AntiAliasing : u8 { None, FXAA, TAA };

// Quality settings that the renderer honours at runtime (settings menu).
struct RenderSettings {
    float renderScale = 1.0f;      // 0.5 .. 2.0
    int shadowQuality = 2;         // 0 off, 1 low, 2 medium, 3 high
    int textureQuality = 2;        // 0 low (512), 1 medium (1024), 2 high (2048 where available)
    bool ambientOcclusion = true;
    bool screenSpaceReflections = true;
    bool bloom = true;
    bool volumetricFog = true;
    AntiAliasing antiAliasing = AntiAliasing::TAA;
    float anisotropy = 8.0f;
    bool filmGrain = true;
    bool chromaticAberration = false;
    float brightness = 1.0f;       // display gamma adjustment (accessibility)
    bool reducedFlashing = false;  // clamps rapid luminance changes
    bool foundFootage = false;     // optional camcorder post effect
};

struct FrameStats {
    u32 drawCalls = 0;
    u32 triangles = 0;
    u32 visibleInstances = 0;
    u32 totalInstances = 0;
    u32 visibleLights = 0;
    u32 shadowMapsRendered = 0;
    double cpuMilliseconds = 0.0;
    double gpuMilliseconds = 0.0;
};

// Blackbody approximation (Tanner Helland fit) returning normalised linear RGB.
glm::vec3 colorTemperatureToRgb(float kelvin);

} // namespace br::render
