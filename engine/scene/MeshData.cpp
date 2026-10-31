#include "engine/scene/MeshData.h"

#include <cmath>

namespace br {

AABB MeshData::bounds() const {
    AABB b;
    for (const auto& v : vertices) {
        b.expand(v.position);
    }
    return b;
}

void MeshData::append(const MeshData& other) {
    const u32 base = static_cast<u32>(vertices.size());
    vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());
    indices.reserve(indices.size() + other.indices.size());
    for (u32 i : other.indices) {
        indices.push_back(base + i);
    }
}

void MeshData::append(const MeshData& other, const glm::mat4& m) {
    const u32 base = static_cast<u32>(vertices.size());
    const glm::mat3 nm = glm::transpose(glm::inverse(glm::mat3(m)));
    const glm::mat3 tm(m);
    const bool flip = glm::determinant(tm) < 0.0f;
    vertices.reserve(vertices.size() + other.vertices.size());
    for (Vertex v : other.vertices) {
        v.position = glm::vec3(m * glm::vec4(v.position, 1.0f));
        v.normal = glm::normalize(nm * v.normal);
        v.tangent = glm::vec4(glm::normalize(tm * glm::vec3(v.tangent)), v.tangent.w);
        vertices.push_back(v);
    }
    indices.reserve(indices.size() + other.indices.size());
    for (usize i = 0; i + 2 < other.indices.size(); i += 3) {
        if (flip) {
            indices.push_back(base + other.indices[i]);
            indices.push_back(base + other.indices[i + 2]);
            indices.push_back(base + other.indices[i + 1]);
        } else {
            indices.push_back(base + other.indices[i]);
            indices.push_back(base + other.indices[i + 1]);
            indices.push_back(base + other.indices[i + 2]);
        }
    }
}

void MeshData::transform(const glm::mat4& m) {
    MeshData tmp;
    tmp.append(*this, m);
    *this = std::move(tmp);
}

void MeshData::computeTangents() {
    std::vector<glm::vec3> tan(vertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> bit(vertices.size(), glm::vec3(0.0f));
    for (usize i = 0; i + 2 < indices.size(); i += 3) {
        const u32 i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
        const Vertex &a = vertices[i0], &b = vertices[i1], &c = vertices[i2];
        const glm::vec3 e1 = b.position - a.position, e2 = c.position - a.position;
        const glm::vec2 d1 = b.uv - a.uv, d2 = c.uv - a.uv;
        float det = d1.x * d2.y - d2.x * d1.y;
        if (std::abs(det) < 1e-12f) {
            det = 1e-12f;
        }
        const float r = 1.0f / det;
        const glm::vec3 t = (e1 * d2.y - e2 * d1.y) * r;
        const glm::vec3 bt = (e2 * d1.x - e1 * d2.x) * r;
        for (u32 idx : {i0, i1, i2}) {
            tan[idx] += t;
            bit[idx] += bt;
        }
    }
    for (usize i = 0; i < vertices.size(); ++i) {
        const glm::vec3 n = vertices[i].normal;
        glm::vec3 t = tan[i] - n * glm::dot(n, tan[i]);
        const float len = glm::length(t);
        if (len < 1e-8f) {
            // Degenerate UVs: build any orthonormal tangent.
            t = std::abs(n.y) < 0.99f ? glm::normalize(glm::cross(glm::vec3(0, 1, 0), n))
                                      : glm::normalize(glm::cross(glm::vec3(1, 0, 0), n));
        } else {
            t /= len;
        }
        const float w = glm::dot(glm::cross(n, t), bit[i]) < 0.0f ? -1.0f : 1.0f;
        vertices[i].tangent = glm::vec4(t, w);
    }
}

void MeshData::computeFlatNormals() {
    MeshData out;
    out.vertices.reserve(indices.size());
    out.indices.reserve(indices.size());
    for (usize i = 0; i + 2 < indices.size(); i += 3) {
        Vertex a = vertices[indices[i]], b = vertices[indices[i + 1]], c = vertices[indices[i + 2]];
        const glm::vec3 n = glm::normalize(glm::cross(b.position - a.position, c.position - a.position));
        a.normal = b.normal = c.normal = n;
        const u32 base = static_cast<u32>(out.vertices.size());
        out.vertices.push_back(a);
        out.vertices.push_back(b);
        out.vertices.push_back(c);
        out.indices.insert(out.indices.end(), {base, base + 1, base + 2});
    }
    *this = std::move(out);
    computeTangents();
}

namespace mesh {

void addQuad(MeshData& out, glm::vec3 o, glm::vec3 u, glm::vec3 v, glm::vec2 uvMin, glm::vec2 uvMax, glm::u8vec4 color) {
    const glm::vec3 n = glm::normalize(glm::cross(u, v));
    const glm::vec3 t = glm::normalize(u);
    const u32 base = static_cast<u32>(out.vertices.size());
    auto push = [&](glm::vec3 p, glm::vec2 uv) {
        Vertex vx;
        vx.position = p;
        vx.normal = n;
        vx.tangent = glm::vec4(t, 1.0f);
        vx.uv = uv;
        vx.color = color;
        out.vertices.push_back(vx);
    };
    push(o, {uvMin.x, uvMin.y});
    push(o + u, {uvMax.x, uvMin.y});
    push(o + u + v, {uvMax.x, uvMax.y});
    push(o + v, {uvMin.x, uvMax.y});
    out.indices.insert(out.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

void addBox(MeshData& out, const AABB& b, float s, BoxFaces f, bool inward, glm::u8vec4 color) {
    const glm::vec3 mn = b.min, mx = b.max, sz = b.size();
    auto quad = [&](glm::vec3 o, glm::vec3 u, glm::vec3 v, glm::vec2 uv0, glm::vec2 uvSize) {
        if (!inward) {
            addQuad(out, o, u, v, uv0, uv0 + uvSize, color);
            return;
        }
        // Inward faces keep the same UV per position (so vertical patterns stay
        // vertical) and only reverse winding and normal.
        const glm::vec3 n = -glm::normalize(glm::cross(u, v));
        const glm::vec4 t(glm::normalize(u), -1.0f);
        const u32 base = static_cast<u32>(out.vertices.size());
        const glm::vec3 p[4] = {o, o + u, o + u + v, o + v};
        const glm::vec2 uv[4] = {uv0, uv0 + glm::vec2(uvSize.x, 0), uv0 + uvSize, uv0 + glm::vec2(0, uvSize.y)};
        for (int i = 0; i < 4; ++i) {
            Vertex vx;
            vx.position = p[i];
            vx.normal = n;
            vx.tangent = t;
            vx.uv = uv[i];
            vx.color = color;
            out.vertices.push_back(vx);
        }
        out.indices.insert(out.indices.end(), {base, base + 2, base + 1, base, base + 3, base + 2});
    };
    // UVs are world-space so adjacent boxes tile continuously.
    if (f.posX) quad({mx.x, mn.y, mx.z}, {0, 0, -sz.z}, {0, sz.y, 0}, glm::vec2(-mx.z, mn.y) * s, glm::vec2(sz.z, sz.y) * s);
    if (f.negX) quad({mn.x, mn.y, mn.z}, {0, 0, sz.z}, {0, sz.y, 0}, glm::vec2(mn.z, mn.y) * s, glm::vec2(sz.z, sz.y) * s);
    if (f.posY) quad({mn.x, mx.y, mx.z}, {sz.x, 0, 0}, {0, 0, -sz.z}, glm::vec2(mn.x, -mx.z) * s, glm::vec2(sz.x, sz.z) * s);
    if (f.negY) quad({mn.x, mn.y, mn.z}, {sz.x, 0, 0}, {0, 0, sz.z}, glm::vec2(mn.x, mn.z) * s, glm::vec2(sz.x, sz.z) * s);
    if (f.posZ) quad({mn.x, mn.y, mx.z}, {sz.x, 0, 0}, {0, sz.y, 0}, glm::vec2(mn.x, mn.y) * s, glm::vec2(sz.x, sz.y) * s);
    if (f.negZ) quad({mx.x, mn.y, mn.z}, {-sz.x, 0, 0}, {0, sz.y, 0}, glm::vec2(-mx.x, mn.y) * s, glm::vec2(sz.x, sz.y) * s);
}

void addCylinder(MeshData& out, glm::vec3 base, float r, float h, int seg, float s, bool caps) {
    const u32 start = static_cast<u32>(out.vertices.size());
    const float circumference = kTwoPi * r;
    for (int i = 0; i <= seg; ++i) {
        const float a = static_cast<float>(i) / static_cast<float>(seg) * kTwoPi;
        const glm::vec3 n(std::cos(a), 0.0f, std::sin(a));
        for (int j = 0; j < 2; ++j) {
            Vertex v;
            v.position = base + n * r + glm::vec3(0, h * static_cast<float>(j), 0);
            v.normal = n;
            v.tangent = glm::vec4(-n.z, 0.0f, n.x, 1.0f);
            v.uv = glm::vec2(static_cast<float>(i) / static_cast<float>(seg) * circumference, h * static_cast<float>(j)) * s;
            out.vertices.push_back(v);
        }
    }
    for (int i = 0; i < seg; ++i) {
        const u32 a = start + static_cast<u32>(i) * 2;
        out.indices.insert(out.indices.end(), {a, a + 1, a + 3, a, a + 3, a + 2});
    }
    if (caps) {
        for (int cap = 0; cap < 2; ++cap) {
            const float y = cap ? h : 0.0f;
            const glm::vec3 n(0, cap ? 1.0f : -1.0f, 0);
            const u32 c = static_cast<u32>(out.vertices.size());
            Vertex cv;
            cv.position = base + glm::vec3(0, y, 0);
            cv.normal = n;
            cv.uv = glm::vec2(base.x, base.z) * s;
            out.vertices.push_back(cv);
            for (int i = 0; i <= seg; ++i) {
                const float a = static_cast<float>(i) / static_cast<float>(seg) * kTwoPi;
                Vertex v;
                v.position = base + glm::vec3(std::cos(a) * r, y, std::sin(a) * r);
                v.normal = n;
                v.uv = glm::vec2(v.position.x, v.position.z) * s;
                out.vertices.push_back(v);
            }
            for (int i = 0; i < seg; ++i) {
                const u32 a = c + 1 + static_cast<u32>(i);
                if (cap) {
                    out.indices.insert(out.indices.end(), {c, a + 1, a});
                } else {
                    out.indices.insert(out.indices.end(), {c, a, a + 1});
                }
            }
        }
    }
}

void addSphere(MeshData& out, glm::vec3 center, float radius, int rings, int segments) {
    const u32 start = static_cast<u32>(out.vertices.size());
    for (int r = 0; r <= rings; ++r) {
        const float phi = static_cast<float>(r) / static_cast<float>(rings) * kPi;
        for (int s = 0; s <= segments; ++s) {
            const float theta = static_cast<float>(s) / static_cast<float>(segments) * kTwoPi;
            const glm::vec3 n(std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta));
            Vertex v;
            v.position = center + n * radius;
            v.normal = n;
            v.tangent = glm::vec4(-std::sin(theta), 0.0f, std::cos(theta), 1.0f);
            v.uv = {static_cast<float>(s) / static_cast<float>(segments), static_cast<float>(r) / static_cast<float>(rings)};
            out.vertices.push_back(v);
        }
    }
    const u32 stride = static_cast<u32>(segments) + 1;
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            const u32 a = start + static_cast<u32>(r) * stride + static_cast<u32>(s);
            const u32 b = a + stride;
            out.indices.insert(out.indices.end(), {a, a + 1, b, a + 1, b + 1, b});
        }
    }
}

MeshData makeBox(glm::vec3 he, float uvScale) {
    MeshData m;
    addBox(m, AABB{-he, he}, uvScale);
    return m;
}

MeshData makeSphere(float radius, int rings, int segments) {
    MeshData m;
    addSphere(m, glm::vec3(0.0f), radius, rings, segments);
    return m;
}

MeshData makeCylinder(float radius, float height, int segments) {
    MeshData m;
    addCylinder(m, glm::vec3(0, -height * 0.5f, 0), radius, height, segments);
    return m;
}

MeshData makePlane(float sx, float sz, float uvScale) {
    MeshData m;
    addQuad(m, {-sx * 0.5f, 0, sz * 0.5f}, {sx, 0, 0}, {0, 0, -sz}, glm::vec2(0.0f), glm::vec2(sx, sz) * uvScale);
    return m;
}

} // namespace mesh
} // namespace br
