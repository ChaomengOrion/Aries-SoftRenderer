/*/// FileName: S_TextureShader.hpp
/// Date: 2025/05/31
/// Author: ChaomengOrion

#pragma once

#include "Shader.hpp"
#include "Texture.hpp"

class TextureShader : public Shader
{
public:
    Vector3f FragmentShader(FragmentPaylod paylod, v2f& data) override {
        // 获取纹理坐标
        Vector2f uv = data.uv;

        // 这里用裸指针加速
        auto mat = static_cast<TextureMaterial*>(paylod.material);
        Texture* texture = mat->texture.get();

        if (texture) {
            return texture->Sample(uv.x(), uv.y());
        }

        // 如果没有纹理，返回默认颜色
        return Vector3f(0.5f, 0.0f, 0.5f); // 紫色
    }
};*/