#include "engine/resources/Image.h"

#include "engine/core/Log.h"

#include <stb_image.h>
#include <stb_image_write.h>

#include <algorithm>
#include <cmath>

namespace br {
namespace {
inline int wrapCoord(int v, int n) {
    const int m = v % n;
    return m < 0 ? m + n : m;
}
} // namespace

Image Image::createU8(int w, int h, int ch) {
    Image img;
    img.width = w;
    img.height = h;
    img.channels = ch;
    img.format = Format::U8;
    img.pixels8.assign(static_cast<usize>(w) * static_cast<usize>(h) * static_cast<usize>(ch), 0);
    return img;
}

Image Image::createF32(int w, int h, int ch) {
    Image img;
    img.width = w;
    img.height = h;
    img.channels = ch;
    img.format = Format::F32;
    img.pixelsF.assign(static_cast<usize>(w) * static_cast<usize>(h) * static_cast<usize>(ch), 0.0f);
    return img;
}

u8* Image::texel8(int x, int y) {
    const usize i = (static_cast<usize>(wrapCoord(y, height)) * static_cast<usize>(width) +
                     static_cast<usize>(wrapCoord(x, width))) * static_cast<usize>(channels);
    return pixels8.data() + i;
}
const u8* Image::texel8(int x, int y) const { return const_cast<Image*>(this)->texel8(x, y); }

float* Image::texelF(int x, int y) {
    const usize i = (static_cast<usize>(wrapCoord(y, height)) * static_cast<usize>(width) +
                     static_cast<usize>(wrapCoord(x, width))) * static_cast<usize>(channels);
    return pixelsF.data() + i;
}
const float* Image::texelF(int x, int y) const { return const_cast<Image*>(this)->texelF(x, y); }

void Image::setRGBA8(int x, int y, glm::vec4 c) {
    u8* p = texel8(x, y);
    for (int i = 0; i < channels && i < 4; ++i) {
        p[i] = static_cast<u8>(std::clamp(c[i], 0.0f, 1.0f) * 255.0f + 0.5f);
    }
}

glm::vec4 Image::getRGBA8(int x, int y) const {
    const u8* p = texel8(x, y);
    glm::vec4 c(0.0f, 0.0f, 0.0f, 1.0f);
    for (int i = 0; i < channels && i < 4; ++i) {
        c[i] = static_cast<float>(p[i]) / 255.0f;
    }
    return c;
}

std::optional<Image> Image::loadFile(const fs::path& path, int desiredChannels) {
    int w = 0, h = 0, n = 0;
    stbi_uc* data = stbi_load(path.string().c_str(), &w, &h, &n, desiredChannels);
    if (!data) {
        BR_LOG_WARN("image", "Failed to load '{}': {}", path.string(), stbi_failure_reason());
        return std::nullopt;
    }
    Image img = createU8(w, h, desiredChannels);
    std::copy(data, data + img.pixels8.size(), img.pixels8.begin());
    stbi_image_free(data);
    return img;
}

bool Image::savePng(const fs::path& path) const {
    std::error_code ec;
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path(), ec);
    }
    if (format == Format::U8) {
        return stbi_write_png(path.string().c_str(), width, height, channels, pixels8.data(), width * channels) != 0;
    }
    std::vector<u8> tmp(pixelsF.size());
    for (usize i = 0; i < tmp.size(); ++i) {
        tmp[i] = static_cast<u8>(std::clamp(pixelsF[i], 0.0f, 1.0f) * 255.0f + 0.5f);
    }
    return stbi_write_png(path.string().c_str(), width, height, channels, tmp.data(), width * channels) != 0;
}

u8 linearToSrgb8(float l) {
    l = std::clamp(l, 0.0f, 1.0f);
    const float s = l <= 0.0031308f ? 12.92f * l : 1.055f * std::pow(l, 1.0f / 2.4f) - 0.055f;
    return static_cast<u8>(s * 255.0f + 0.5f);
}

float srgb8ToLinear(u8 v) {
    const float s = static_cast<float>(v) / 255.0f;
    return s <= 0.04045f ? s / 12.92f : std::pow((s + 0.055f) / 1.055f, 2.4f);
}

} // namespace br
