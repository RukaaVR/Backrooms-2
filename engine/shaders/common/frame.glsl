// Shared per-frame data. Mirrors FrameUniforms in engine/renderer/Renderer.cpp.
layout(std140, binding = 0) uniform FrameData {
    mat4 uView;
    mat4 uProj;
    mat4 uViewProj;
    mat4 uInvViewProj;
    vec4 uCameraPosTime;     // xyz camera position, w time (s)
    vec4 uAmbient;           // rgb ambient irradiance
    vec4 uFogColorDensity;   // rgb fog colour, w density
    vec4 uFogParams;         // x height falloff, y fog height, z light count, w exposure
    vec4 uSunDirIntensity;   // xyz direction light travels, w intensity
    vec4 uSunColor;
    vec4 uSkyZenith;
    vec4 uSkyHorizon;        // w = has sky
};

struct GpuLight {
    vec4 posRange;        // xyz position, w range
    vec4 colorIntensity;  // rgb colour, w intensity
    vec4 dirType;         // xyz direction, w type (0 point, 1 spot, 2 rect)
    vec4 spotRect;        // x cos inner, y cos outer, z rect width, w rect height
    vec4 right;           // xyz rect tangent, w source radius
};

layout(std430, binding = 1) readonly buffer LightBuffer {
    GpuLight uLights[];
};
