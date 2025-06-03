/*/// FileName: S_NormalTestShader.hpp
/// Date: 2025/05/31
/// Author: ChaomengOrion

#pragma once

#include "Shader.hpp"

class NormalTestShader : public Shader
{
    Vector3f FragmentShader([[maybe_unused]] FragmentPaylod paylod, v2f& data) override {
        // TEST
        Vector3f viewNormal = data.normal;
        Vector3f returnColor = (viewNormal + Vector3f(1.0f, 1.0f, 1.0f)) / 2.0f;
        return returnColor;
    }
};*/