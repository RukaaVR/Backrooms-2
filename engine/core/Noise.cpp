#include "engine/core/Noise.h"

#include "engine/core/Random.h"

#include <cmath>
#include <limits>

namespace br::noise {
namespace {

inline u32 hashU32(i32 x, i32 y, u32 seed) {
    return static_cast<u32>(hash2i(seed, x, y));
}

inline u32 hashU32(i32 x, i32 y, i32 z, u32 seed) {
    return static_cast<u32>(hash3i(seed, x, y, z));
}

inline float fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

inline int wrap(int v, int period) {
    if (period <= 0) {
        return v;
    }
    const int m = v % period;
    return m < 0 ? m + period : m;
}

// 2D gradients on the unit circle (16 directions) avoid the axis-aligned
// artefacts of the classic 4-gradient Perlin set.
inline glm::vec2 grad2(u32 h) {
    const float a = static_cast<float>(h & 15u) * (6.28318530718f / 16.0f);
    return {std::cos(a), std::sin(a)};
}

inline float dotGrad3(u32 h, glm::vec3 d) {
    switch (h % 12u) {
    case 0: return d.x + d.y;
    case 1: return -d.x + d.y;
    case 2: return d.x - d.y;
    case 3: return -d.x - d.y;
    case 4: return d.x + d.z;
    case 5: return -d.x + d.z;
    case 6: return d.x - d.z;
    case 7: return -d.x - d.z;
    case 8: return d.y + d.z;
    case 9: return -d.y + d.z;
    case 10: return d.y - d.z;
    default: return -d.y - d.z;
    }
}

float gradient2Impl(glm::vec2 p, glm::ivec2 period, u32 seed) {
    const glm::vec2 pf = glm::floor(p);
    const glm::ivec2 pi(static_cast<int>(pf.x), static_cast<int>(pf.y));
    const glm::vec2 f = p - pf;
    auto corner = [&](int ox, int oy) {
        const u32 h = hashU32(wrap(pi.x + ox, period.x), wrap(pi.y + oy, period.y), seed);
        return glm::dot(grad2(h), f - glm::vec2(ox, oy));
    };
    const float u = fade(f.x);
    const float v = fade(f.y);
    const float a = glm::mix(corner(0, 0), corner(1, 0), u);
    const float b = glm::mix(corner(0, 1), corner(1, 1), u);
    return glm::mix(a, b, v) * 1.41421356f;
}

CellularResult cellular2Impl(glm::vec2 p, glm::ivec2 period, u32 seed) {
    const glm::vec2 pf = glm::floor(p);
    const glm::ivec2 pi(static_cast<int>(pf.x), static_cast<int>(pf.y));
    CellularResult r;
    r.f1 = r.f2 = std::numeric_limits<float>::max();
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            const int cx = wrap(pi.x + x, period.x);
            const int cy = wrap(pi.y + y, period.y);
            const u32 h = hashU32(cx, cy, seed);
            const glm::vec2 feature = glm::vec2(pi.x + x, pi.y + y) +
                                      glm::vec2(hashToUnitFloat(splitmix64(h)), hashToUnitFloat(splitmix64(h + 1)));
            const float d = glm::length(feature - p);
            if (d < r.f1) {
                r.f2 = r.f1;
                r.f1 = d;
                r.cellId = h;
            } else if (d < r.f2) {
                r.f2 = d;
            }
        }
    }
    return r;
}

} // namespace

float gradient2(glm::vec2 p, u32 seed) { return gradient2Impl(p, {0, 0}, seed); }

float gradient2Tiled(glm::vec2 p, glm::ivec2 period, u32 seed) { return gradient2Impl(p, period, seed); }

float gradient3(glm::vec3 p, u32 seed) {
    const glm::vec3 pf = glm::floor(p);
    const glm::ivec3 pi(static_cast<int>(pf.x), static_cast<int>(pf.y), static_cast<int>(pf.z));
    const glm::vec3 f = p - pf;
    auto corner = [&](int ox, int oy, int oz) {
        return dotGrad3(hashU32(pi.x + ox, pi.y + oy, pi.z + oz, seed), f - glm::vec3(ox, oy, oz));
    };
    const float u = fade(f.x), v = fade(f.y), w = fade(f.z);
    const float x00 = glm::mix(corner(0, 0, 0), corner(1, 0, 0), u);
    const float x10 = glm::mix(corner(0, 1, 0), corner(1, 1, 0), u);
    const float x01 = glm::mix(corner(0, 0, 1), corner(1, 0, 1), u);
    const float x11 = glm::mix(corner(0, 1, 1), corner(1, 1, 1), u);
    return glm::mix(glm::mix(x00, x10, v), glm::mix(x01, x11, v), w);
}

float value2(glm::vec2 p, u32 seed) {
    const glm::vec2 pf = glm::floor(p);
    const glm::ivec2 pi(static_cast<int>(pf.x), static_cast<int>(pf.y));
    const glm::vec2 f = p - pf;
    auto v = [&](int ox, int oy) { return hashToUnitFloat(splitmix64(hashU32(pi.x + ox, pi.y + oy, seed))); };
    const float u = fade(f.x), w = fade(f.y);
    return glm::mix(glm::mix(v(0, 0), v(1, 0), u), glm::mix(v(0, 1), v(1, 1), u), w);
}

float fbm2(glm::vec2 p, u32 seed, const FbmParams& params) {
    float sum = 0.0f, amp = 0.5f, norm = 0.0f;
    for (int i = 0; i < params.octaves; ++i) {
        sum += amp * gradient2(p, seed + static_cast<u32>(i) * 1013u);
        norm += amp;
        p *= params.lacunarity;
        amp *= params.gain;
    }
    return sum / norm;
}

float fbm3(glm::vec3 p, u32 seed, const FbmParams& params) {
    float sum = 0.0f, amp = 0.5f, norm = 0.0f;
    for (int i = 0; i < params.octaves; ++i) {
        sum += amp * gradient3(p, seed + static_cast<u32>(i) * 1013u);
        norm += amp;
        p *= params.lacunarity;
        amp *= params.gain;
    }
    return sum / norm;
}

float fbm2Tiled(glm::vec2 p, glm::ivec2 period, u32 seed, const FbmParams& params) {
    float sum = 0.0f, amp = 0.5f, norm = 0.0f;
    glm::ivec2 per = period;
    for (int i = 0; i < params.octaves; ++i) {
        sum += amp * gradient2Tiled(p, per, seed + static_cast<u32>(i) * 1013u);
        norm += amp;
        p *= params.lacunarity;
        // Tiling only survives integer lacunarity; callers use 2.0 for tiled noise.
        per = glm::ivec2(glm::vec2(per) * params.lacunarity);
        amp *= params.gain;
    }
    return sum / norm;
}

float ridged2(glm::vec2 p, u32 seed, const FbmParams& params) {
    float sum = 0.0f, amp = 0.5f, norm = 0.0f;
    for (int i = 0; i < params.octaves; ++i) {
        const float n = 1.0f - std::abs(gradient2(p, seed + static_cast<u32>(i) * 1013u));
        sum += amp * n * n;
        norm += amp;
        p *= params.lacunarity;
        amp *= params.gain;
    }
    return sum / norm;
}

CellularResult cellular2(glm::vec2 p, u32 seed) { return cellular2Impl(p, {0, 0}, seed); }

CellularResult cellular2Tiled(glm::vec2 p, glm::ivec2 period, u32 seed) { return cellular2Impl(p, period, seed); }

CellularResult cellular3(glm::vec3 p, u32 seed) {
    const glm::vec3 pf = glm::floor(p);
    const glm::ivec3 pi(static_cast<int>(pf.x), static_cast<int>(pf.y), static_cast<int>(pf.z));
    CellularResult r;
    r.f1 = r.f2 = std::numeric_limits<float>::max();
    for (int z = -1; z <= 1; ++z) {
        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                const u32 h = hashU32(pi.x + x, pi.y + y, pi.z + z, seed);
                const glm::vec3 feature = glm::vec3(pi.x + x, pi.y + y, pi.z + z) +
                                          glm::vec3(hashToUnitFloat(splitmix64(h)), hashToUnitFloat(splitmix64(h + 1)),
                                                    hashToUnitFloat(splitmix64(h + 2)));
                const float d = glm::length(feature - p);
                if (d < r.f1) {
                    r.f2 = r.f1;
                    r.f1 = d;
                    r.cellId = h;
                } else if (d < r.f2) {
                    r.f2 = d;
                }
            }
        }
    }
    return r;
}

} // namespace br::noise
