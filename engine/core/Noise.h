#pragma once

#include "engine/core/Types.h"

#include <glm/glm.hpp>

namespace br::noise {

// Seeded, deterministic coherent noise. All functions return values roughly in
// [-1, 1] (gradient noise) or [0, 1] (cellular / value noise) and are safe to
// call concurrently from generator worker threads.

float gradient2(glm::vec2 p, u32 seed);
float gradient3(glm::vec3 p, u32 seed);

// Gradient noise that tiles with the given integer period, for seamless textures.
float gradient2Tiled(glm::vec2 p, glm::ivec2 period, u32 seed);

float value2(glm::vec2 p, u32 seed);

struct FbmParams {
    int octaves = 5;
    float lacunarity = 2.0f;
    float gain = 0.5f;
};

float fbm2(glm::vec2 p, u32 seed, const FbmParams& params = {});
float fbm3(glm::vec3 p, u32 seed, const FbmParams& params = {});
float fbm2Tiled(glm::vec2 p, glm::ivec2 period, u32 seed, const FbmParams& params = {});
float ridged2(glm::vec2 p, u32 seed, const FbmParams& params = {});

struct CellularResult {
    float f1 = 0.0f;   // distance to nearest feature point
    float f2 = 0.0f;   // distance to second-nearest feature point
    u32 cellId = 0;    // stable id of the nearest cell (for per-cell variation)
};

CellularResult cellular2(glm::vec2 p, u32 seed);
CellularResult cellular2Tiled(glm::vec2 p, glm::ivec2 period, u32 seed);
CellularResult cellular3(glm::vec3 p, u32 seed);

} // namespace br::noise
