/// FileName: Texture.cpp
/// Date: 2025/06/01
/// Author: ChaomengOrion

//#define STB_IMAGE_IMPLEMENTATION // IMGuiFileDialog 定义了这个
#include "stb_image.h"
#include "Texture.hpp"
#include <cmath>
#include <algorithm>

bool Texture::LoadFromFile(const std::string& filename) {
    unsigned char* img = stbi_load(
        filename.c_str(), &width, &height, &channels, 0);
    if (!img) {
        return false;
    }
    data.assign(img, img + width * height * channels);
    stbi_image_free(img);
    return true;
}

Vector3f Texture::Sample(float u, float v) const {
    if (data.empty() || width <= 0 || height <= 0) {
        return Vector3f(1.0f, 1.0f, 1.0f);
    }
    // 循环 UV
    u = u - std::floor(u);
    v = v - std::floor(v);
    // 翻转 v 轴（stbi 原点在左上）
    v = 1.0f - v;

    int x = std::clamp(int(u * width), 0, width - 1);
    int y = std::clamp(int(v * height), 0, height - 1);
    int idx = (y * width + x) * channels;

    float r = data[idx + 0] / 255.0f;
    float g = (channels >= 2 ? data[idx + 1] : data[idx + 0]) / 255.0f;
    float b = (channels >= 3 ? data[idx + 2] : data[idx + 0]) / 255.0f;
    return Vector3f(r, g, b);
}