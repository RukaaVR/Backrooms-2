#include "../common/frame.glsl"

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aTangent;
layout(location = 3) in vec2 aUv;
layout(location = 4) in vec4 aColor;

uniform mat4 uModel;
uniform mat3 uNormalMatrix;

out VS_OUT {
    vec3 worldPos;
    vec3 normal;
    vec4 tangent;
    vec2 uv;
    vec4 color;
} vs;

void main() {
    vec4 world = uModel * vec4(aPosition, 1.0);
    vs.worldPos = world.xyz;
    vs.normal = normalize(uNormalMatrix * aNormal);
    vs.tangent = vec4(normalize(mat3(uModel) * aTangent.xyz), aTangent.w);
    vs.uv = aUv;
    vs.color = aColor;
    gl_Position = uViewProj * world;
}
