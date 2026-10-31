#pragma once

#include "engine/core/Math.h"
#include "engine/core/Types.h"

#include <glm/glm.hpp>

#include <vector>

namespace br {

// Canonical static vertex. CPU-only type (no GL dependency) so that worker
// threads can build geometry for streamed chunks.
//   color.rgb : per-vertex tint (sRGB-encoded, multiplied with albedo)
//   color.a   : baked ambient occlusion / cavity term (1 = unoccluded)
struct Vertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f}; // xyz tangent, w handedness
    glm::vec2 uv{0.0f};
    glm::u8vec4 color{255, 255, 255, 255};
};
static_assert(sizeof(Vertex) == 52, "Vertex layout is mirrored in the renderer's VAO format");

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;

    bool empty() const { return indices.empty(); }
    void clear() {
        vertices.clear();
        indices.clear();
    }
    void reserve(usize v, usize i) {
        vertices.reserve(v);
        indices.reserve(i);
    }
    AABB bounds() const;
    void append(const MeshData& other);
    void append(const MeshData& other, const glm::mat4& transform);
    void transform(const glm::mat4& m);
    // MikkTSpace-style per-triangle accumulation; required for normal mapping.
    void computeTangents();
    void computeFlatNormals();
};

// Procedural geometry helpers. UVs default to world-space metres * uvScale so
// tiled materials keep a constant texel density regardless of object size.
namespace mesh {

struct BoxFaces {
    bool posX = true, negX = true, posY = true, negY = true, posZ = true, negZ = true;
};

// Axis-aligned box. `inward` flips winding/normals (for rooms seen from inside).
void addBox(MeshData& out, const AABB& box, float uvScale = 1.0f, BoxFaces faces = {}, bool inward = false,
            glm::u8vec4 color = {255, 255, 255, 255});

// Quad from origin along two edge vectors; normal = normalize(cross(u, v)).
void addQuad(MeshData& out, glm::vec3 origin, glm::vec3 edgeU, glm::vec3 edgeV, glm::vec2 uvMin, glm::vec2 uvMax,
             glm::u8vec4 color = {255, 255, 255, 255});

void addCylinder(MeshData& out, glm::vec3 base, float radius, float height, int segments, float uvScale = 1.0f,
                 bool caps = true);
void addSphere(MeshData& out, glm::vec3 center, float radius, int rings, int segments);

// Unit primitives centred at the origin, used for props and debug geometry.
MeshData makeBox(glm::vec3 halfExtents, float uvScale = 1.0f);
MeshData makeSphere(float radius, int rings = 16, int segments = 24);
MeshData makeCylinder(float radius, float height, int segments = 24);
MeshData makePlane(float sizeX, float sizeZ, float uvScale = 1.0f);

} // namespace mesh
} // namespace br
