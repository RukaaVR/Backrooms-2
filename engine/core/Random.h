#pragma once

#include "engine/core/Types.h"

#include <glm/glm.hpp>

#include <cmath>
#include <string_view>

namespace br {

// Stateless integer hashing used for deterministic procedural generation.
// Everything derived from a world seed must flow through these so that a given
// (seed, coordinate) pair always reproduces identical content on any platform.
constexpr u64 splitmix64(u64 x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

constexpr u64 hashCombine(u64 a, u64 b) { return splitmix64(a ^ (splitmix64(b) + 0x9E3779B97F4A7C15ull + (a << 6) + (a >> 2))); }

constexpr u64 hash2i(u64 seed, i32 x, i32 y) {
    return hashCombine(hashCombine(seed, static_cast<u64>(static_cast<u32>(x))), static_cast<u64>(static_cast<u32>(y)));
}

constexpr u64 hash3i(u64 seed, i32 x, i32 y, i32 z) {
    return hashCombine(hash2i(seed, x, y), static_cast<u64>(static_cast<u32>(z)));
}

constexpr u64 hashString(std::string_view s) {
    u64 h = 0xCBF29CE484222325ull; // FNV-1a
    for (char c : s) {
        h ^= static_cast<u8>(c);
        h *= 0x100000001B3ull;
    }
    return h;
}

// Maps a hash to [0,1).
constexpr float hashToUnitFloat(u64 h) { return static_cast<float>(h >> 40) * (1.0f / 16777216.0f); }

// PCG32 (O'Neill). Small, fast, statistically solid and fully deterministic.
class Rng {
public:
    constexpr explicit Rng(u64 seed = 0x853C49E6748FEA9Bull, u64 stream = 0xDA3E39CB94B95BDBull) {
        m_state = 0;
        m_inc = (stream << 1u) | 1u;
        nextU32();
        m_state += seed;
        nextU32();
    }

    constexpr u32 nextU32() {
        const u64 old = m_state;
        m_state = old * 6364136223846793005ull + m_inc;
        const u32 xorshifted = static_cast<u32>(((old >> 18u) ^ old) >> 27u);
        const u32 rot = static_cast<u32>(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((~rot + 1u) & 31u));
    }

    constexpr u64 nextU64() { return (static_cast<u64>(nextU32()) << 32) | nextU32(); }

    // Uniform in [0, 1).
    constexpr float nextFloat() { return static_cast<float>(nextU32() >> 8) * (1.0f / 16777216.0f); }

    constexpr float range(float lo, float hi) { return lo + (hi - lo) * nextFloat(); }

    // Uniform integer in [lo, hi] inclusive. Uses Lemire's unbiased bounded method.
    constexpr i32 rangeInt(i32 lo, i32 hi) {
        if (hi <= lo) {
            return lo;
        }
        const u32 bound = static_cast<u32>(hi - lo) + 1u;
        u64 m = static_cast<u64>(nextU32()) * bound;
        u32 l = static_cast<u32>(m);
        if (l < bound) {
            const u32 t = (~bound + 1u) % bound;
            while (l < t) {
                m = static_cast<u64>(nextU32()) * bound;
                l = static_cast<u32>(m);
            }
        }
        return lo + static_cast<i32>(m >> 32);
    }

    constexpr bool chance(float probability) { return nextFloat() < probability; }

    glm::vec2 unitDisk() {
        const float r = std::sqrt(nextFloat());
        const float a = nextFloat() * 6.28318530718f;
        return {r * std::cos(a), r * std::sin(a)};
    }

    glm::vec3 unitSphere() {
        const float z = range(-1.0f, 1.0f);
        const float a = nextFloat() * 6.28318530718f;
        const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
        return {r * std::cos(a), r * std::sin(a), z};
    }

private:
    u64 m_state = 0;
    u64 m_inc = 0;
};

} // namespace br
