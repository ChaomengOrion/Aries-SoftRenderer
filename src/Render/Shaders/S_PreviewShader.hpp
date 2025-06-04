/// FileName: S_PreviewShader.hpp
/// Date: 2025/06/04
/// Author: ChaomengOrion

#pragma once

#include "Shader.hpp"

namespace aries::shader {
    template<> // 特化v2f类型
    struct v2f<class PreviewShader> {
        Vector4f viewport;   // 屏幕空间坐标（viewport * MVP * position）
        Vector4f viewPos;    // 视图空间坐标（V * M * pos）
        Vector3f normal;     // 世界/视图空间法线
    };

    class PreviewShader : public ShaderBase<PreviewShader> {
    public:
        constexpr static ShaderType GetTypeImpl() {
            return ShaderType::Preview; // 返回着色器类型
        }

        inline static v2f_t VertexShaderImpl(const VertexPaylod_t& paylod, const a2v& data) {
            v2f_t v2fData;
            v2fData.viewport = paylod.mvp * data.position; // 计算裁剪空间坐标
            v2fData.viewPos = paylod.view * data.position; // 计算视图空间坐标
            v2fData.normal = (paylod.view * Vector4f(data.normal.x(), data.normal.y(), data.normal.z(), 0.0f)).template head<3>().normalized(); // 法线转换
            return v2fData;
        }

        // CRTP实现 - 编译器知道确切类型，可内联优化
        inline static Vector3f FragmentShaderImpl(const FragmentPaylod_t& paylod, const v2f_t& data) {
            Vector3f N = data.normal;
            Vector3f L = -(data.viewPos.head<3>()).normalized();

            float diff = std::max(N.dot(L), 0.0f);

            return Vector3f(diff, diff, diff);
        }
    };
}