#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace br {

inline constexpr float kPi = 3.14159265358979323846f;
inline constexpr float kTwoPi = 2.0f * kPi;

struct AABB {
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{std::numeric_limits<float>::lowest()};

    static AABB fromCenterExtents(glm::vec3 c, glm::vec3 halfExtents) { return {c - halfExtents, c + halfExtents}; }
    bool valid() const { return min.x <= max.x && min.y <= max.y && min.z <= max.z; }
    glm::vec3 center() const { return (min + max) * 0.5f; }
    glm::vec3 extents() const { return (max - min) * 0.5f; }
    glm::vec3 size() const { return max - min; }
    void expand(glm::vec3 p) {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }
    void expand(const AABB& o) {
        min = glm::min(min, o.min);
        max = glm::max(max, o.max);
    }
    bool contains(glm::vec3 p) const {
        return p.x >= min.x && p.y >= min.y && p.z >= min.z && p.x <= max.x && p.y <= max.y && p.z <= max.z;
    }
    bool intersects(const AABB& o) const {
        return min.x <= o.max.x && max.x >= o.min.x && min.y <= o.max.y && max.y >= o.min.y && min.z <= o.max.z &&
               max.z >= o.min.z;
    }
    AABB transformed(const glm::mat4& m) const {
        // Arvo's method: transform extents by the absolute rotation matrix.
        const glm::vec3 c = glm::vec3(m * glm::vec4(center(), 1.0f));
        const glm::vec3 e = extents();
        const glm::mat3 r(m);
        const glm::vec3 ne = glm::abs(r[0]) * e.x + glm::abs(r[1]) * e.y + glm::abs(r[2]) * e.z;
        return {c - ne, c + ne};
    }
};

struct Plane {
    glm::vec3 normal{0, 1, 0};
    float d = 0.0f; // dot(normal, p) + d = 0
    float distance(glm::vec3 p) const { return glm::dot(normal, p) + d; }
};

// View frustum extracted from a combined view-projection matrix (Gribb/Hartmann).
struct Frustum {
    std::array<Plane, 6> planes{};

    static Frustum fromMatrix(const glm::mat4& vp) {
        Frustum f;
        const glm::mat4 m = glm::transpose(vp);
        const glm::vec4 rows[6] = {m[3] + m[0], m[3] - m[0], m[3] + m[1], m[3] - m[1], m[3] + m[2], m[3] - m[2]};
        for (int i = 0; i < 6; ++i) {
            const float len = glm::length(glm::vec3(rows[i]));
            f.planes[i].normal = glm::vec3(rows[i]) / len;
            f.planes[i].d = rows[i].w / len;
        }
        return f;
    }

    bool intersects(const AABB& box) const {
        for (const Plane& p : planes) {
            const glm::vec3 positive{p.normal.x >= 0 ? box.max.x : box.min.x, p.normal.y >= 0 ? box.max.y : box.min.y,
                                     p.normal.z >= 0 ? box.max.z : box.min.z};
            if (p.distance(positive) < 0.0f) {
                return false;
            }
        }
        return true;
    }

    bool intersectsSphere(glm::vec3 c, float r) const {
        for (const Plane& p : planes) {
            if (p.distance(c) < -r) {
                return false;
            }
        }
        return true;
    }
};

struct Transform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    glm::mat4 matrix() const {
        glm::mat4 m = glm::mat4_cast(rotation);
        m[0] *= scale.x;
        m[1] *= scale.y;
        m[2] *= scale.z;
        m[3] = glm::vec4(position, 1.0f);
        return m;
    }

    static Transform interpolate(const Transform& a, const Transform& b, float t) {
        return {glm::mix(a.position, b.position, t), glm::slerp(a.rotation, b.rotation, t), glm::mix(a.scale, b.scale, t)};
    }
};

inline float smoothstep01(float x) {
    x = std::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

// Frame-rate independent exponential smoothing toward a target.
inline float damp(float current, float target, float lambda, float dt) {
    return glm::mix(current, target, 1.0f - std::exp(-lambda * dt));
}
inline glm::vec3 damp(glm::vec3 current, glm::vec3 target, float lambda, float dt) {
    return glm::mix(current, target, 1.0f - std::exp(-lambda * dt));
}

} // namespace br
