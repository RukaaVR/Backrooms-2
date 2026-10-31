#include "../common/color.glsl"

in vec2 vUv;
layout(binding = 0) uniform sampler2D uHdr;
uniform float uExposure;
uniform float uBrightness;
uniform float uSaturation;
uniform float uContrast;
uniform float uVignette;
uniform float uGrain;
uniform float uTime;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 hdr = texture(uHdr, vUv).rgb * uExposure;
    vec3 c = tonemapACES(hdr);
    float l = luminance(c);
    c = mix(vec3(l), c, uSaturation);
    c = clamp((c - 0.5) * uContrast + 0.5, 0.0, 1.0);
    vec2 d = vUv - 0.5;
    c *= 1.0 - uVignette * dot(d, d) * 2.0;
    c = linearToSrgb(c);
    c = pow(c, vec3(1.0 / max(uBrightness, 0.1)));
    float n = hash12(gl_FragCoord.xy + fract(uTime) * 1000.0) - 0.5;
    c += n * uGrain;
    outColor = vec4(c, 1.0);
}
