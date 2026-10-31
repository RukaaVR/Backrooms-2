#include "../common/frame.glsl"
#include "../common/pbr.glsl"

in VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec4 tangent;
    vec2 uv;
    vec4 color;
} fs;

layout(binding = 0) uniform sampler2D uAlbedoTex;
layout(binding = 1) uniform sampler2D uNormalTex;
layout(binding = 2) uniform sampler2D uOrmTex;
layout(binding = 3) uniform sampler2D uEmissiveTex;

uniform vec4 uBaseColor;
uniform float uRoughness;
uniform float uMetallic;
uniform float uNormalStrength;
uniform float uOcclusionStrength;
uniform vec3 uEmissive;
uniform int uHasEmissiveTex;
uniform vec2 uUvScale;
uniform float uAlphaCutoff;
uniform vec4 uInstanceParams;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 uv = fs.uv * uUvScale;
    vec4 albedoSample = texture(uAlbedoTex, uv);
    if (albedoSample.a < uAlphaCutoff) discard;
    vec3 tint = fs.color.rgb * fs.color.rgb; // vertex tint is sRGB-encoded
    vec3 albedo = albedoSample.rgb * uBaseColor.rgb * tint;
    vec3 orm = texture(uOrmTex, uv).rgb;
    float ao = mix(1.0, orm.r, uOcclusionStrength) * fs.color.a;
    float roughness = clamp(orm.g * uRoughness, 0.03, 1.0);
    float metallic = clamp(orm.b * uMetallic, 0.0, 1.0);

    vec3 N = normalize(fs.normal);
    if (!gl_FrontFacing) N = -N;
    vec3 T = normalize(fs.tangent.xyz - N * dot(N, fs.tangent.xyz));
    vec3 B = cross(N, T) * fs.tangent.w;
    vec3 tn = texture(uNormalTex, uv).xyz * 2.0 - 1.0;
    tn.xy *= uNormalStrength;
    N = normalize(mat3(T, B, N) * tn);

    vec3 V = normalize(uCameraPosTime.xyz - fs.worldPos);
    vec3 color = vec3(0.0);
    int lightCount = int(uFogParams.z);
    for (int i = 0; i < lightCount; ++i) {
        GpuLight l = uLights[i];
        vec3 toL = l.posRange.xyz - fs.worldPos;
        float d2 = dot(toL, toL);
        if (d2 > l.posRange.w * l.posRange.w) continue;
        vec3 L = toL * inversesqrt(d2);
        float atten = distanceAttenuation(d2, l.posRange.w);
        int type = int(l.dirType.w);
        if (type == 1) {
            float cd = dot(-L, l.dirType.xyz);
            atten *= smoothstep(l.spotRect.y, l.spotRect.x, cd);
        } else if (type == 2) {
            // Rect lights emit into the hemisphere they face (Lambertian emitter).
            atten *= clamp(dot(-L, l.dirType.xyz), 0.0, 1.0);
        }
        color += evaluateBRDF(N, V, L, albedo, roughness, metallic) * l.colorIntensity.rgb * l.colorIntensity.w * atten * 8.0;
    }
    if (uSunDirIntensity.w > 0.0) {
        color += evaluateBRDF(N, V, -normalize(uSunDirIntensity.xyz), albedo, roughness, metallic) * uSunColor.rgb * uSunDirIntensity.w;
    }
    color += uAmbient.rgb * albedo * ao;
    vec3 emissive = uEmissive;
    if (uHasEmissiveTex == 1) emissive *= texture(uEmissiveTex, uv).rgb;
    color += emissive * uInstanceParams.x;

    // Exponential distance fog.
    float dist = length(uCameraPosTime.xyz - fs.worldPos);
    float fog = 1.0 - exp(-uFogColorDensity.w * dist);
    color = mix(color, uFogColorDensity.rgb, fog);

    outColor = vec4(color, 1.0);
}
