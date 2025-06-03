/*/// FileName: S_PreviewShader.hpp
/// Date: 2025/05/31
/// Author: ChaomengOrion

#pragma once

#include "Shader.hpp"

using Eigen::Vector3f;
using Eigen::Vector4f;

class PreviewShader : public Shader
{
    Vector3f FragmentShader([[maybe_unused]] FragmentPaylod paylod, v2f& data) override {
        // 法线方向
        Vector3f N = data.normal;

        // 光照方向：从片元指向摄像机
        Vector3f L = -(data.viewPos.head<3>()).normalized();

        // 漫反射强度
        float diff = std::max(N.dot(L), 0.0f);

        // 返回灰度值
        return Vector3f(diff, diff, diff);
    }
};*/