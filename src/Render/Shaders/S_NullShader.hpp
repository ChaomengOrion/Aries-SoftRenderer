/*/// FileName: S_NullShader.hpp
/// Date: 2025/05/31
/// Author: ChaomengOrion

#pragma once

#include "Shader.hpp"

class NullShader : public Shader
{
    Vector3f FragmentShader([[maybe_unused]] FragmentPayload payload, [[maybe_unused]] v2f& data) override {
        // 返回一个默认颜色（紫色）
        return Vector3f(0.5f, 0.0f, 0.5f); // 紫色
    }
};*/