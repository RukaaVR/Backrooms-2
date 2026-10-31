#pragma once

#include "engine/core/FileSystem.h"
#include "engine/core/Types.h"

#include <glm/glm.hpp>

#include <optional>
#include <vector>

namespace br {

// CPU-side image. 8-bit images store one byte per channel; float images store
// one f32 per channel (converted to half floats on GPU upload).
struct Image {
    enum class Format : u8 { U8, F32 };

    int width = 0;
    int height = 0;
    int channels = 4;
    Format format = Format::U8;
    std::vector<u8> pixels8;
    std::vector<float> pixelsF;

    static Image createU8(int w, int h, int channels);
    static Image createF32(int w, int h, int channels);

    bool empty() const { return width == 0 || height == 0; }
    usize pixelCount() const { return static_cast<usize>(width) * static_cast<usize>(height); }

    // Accessors use wrap-around addressing (all procedural textures tile).
    u8* texel8(int x, int y);
    const u8* texel8(int x, int y) const;
    float* texelF(int x, int y);
    const float* texelF(int x, int y) const;

    void setRGBA8(int x, int y, glm::vec4 rgba01);
    glm::vec4 getRGBA8(int x, int y) const;

    static std::optional<Image> loadFile(const fs::path& path, int desiredChannels = 4);
    bool savePng(const fs::path& path) const; // U8 only; F32 is clamped to [0,1]
};

// Encodes linear [0,1] to 8-bit sRGB and back (exact IEC 61966-2-1 curve).
u8 linearToSrgb8(float linear);
float srgb8ToLinear(u8 srgb);

} // namespace br
