/// FileName: S_BlinnPhongShader.hpp
/// Date: 2025/05/31
/// Author: ChaomengOrion

#pragma once

#include "Shader.hpp"
#include "../Texture.hpp"
#include "../../Scene.hpp"

namespace aries::shader {
    template<> // 特化v2f类型
    struct v2f<BlinnPhongShader> {
        Vector4f viewport;   // 屏幕空间坐标（viewport * MVP * position）
        Vector4f viewPos;    // 视图空间坐标（V * M * pos）
        Vector3f normal;     // 世界/视图空间法线
        Vector2f uv;         // 纹理坐标
    };

    class BlinnPhongShader : public ShaderBase<BlinnPhongShader> {
    public:
        constexpr static ShaderType GetTypeImpl() {
            return ShaderType::BlinnPhong; // 返回着色器类型
        }

        // CRTP实现 - 编译器知道确切类型，可内联优化
        inline static Vector3f FragmentShaderImpl(const FragmentPaylod_t& paylod, const v2f_t& data) {
            auto& property = *paylod.property; // 获取着色器属性

            // 获取纹理坐标
            Vector2f uv = data.uv;

            // 这里用裸指针加速，避免 shared_ptr 原子操作的开销，经过测试，性能提升明显（2025-6-1测试 60fps -> 100fps 13万顶点）
        
            Texture* texture = property.texture.get();

            Vector3f color = texture ? texture->Sample(uv.x(), uv.y()) : Vector3f(1.f, 1.f, 1.f);

            auto light = paylod.scene->mainLight.get();

            [[unlikely]]
            if (!light || !texture) {
                return color; // 如果没有光源，直接返回颜色
            }

            // Blinn-Phong 着色模型
            Vector3f N = data.normal.normalized(); // 法线，相机空间
            Vector3f L = -(paylod.view * Vector4f(light->direction.x(), light->direction.y(), light->direction.z(), 0)).head<3>().normalized(); // 光照方向
            Vector3f V = -data.viewPos.head<3>().normalized(); // 视线方向
            Vector3f H = (L + V).normalized(); // 半程向量
            float diff = std::max(N.dot(L), 0.0f); // 漫反射分量
            float spec = std::pow(std::max(N.dot(H), 0.0f), property.shininess); // 镜面反射分量
        
            // 计算最终颜色
            // 环境光、漫反射和镜面反射
            Vector3f ambient = property.ambient * property.ambientIntensity; // 环境光
            Vector3f diffuse = property.diffuse * property.diffuseIntensity * diff; // 漫反射光
            Vector3f specular = property.specular * property.specularIntensity * spec; // 镜面反射光

            Vector3f finalColor = color.cwiseProduct(ambient + diffuse + specular); // 计算最终颜色
            finalColor = finalColor.cwiseMin(Vector3f(1.0f, 1.0f, 1.0f)); // 确保颜色不超过1.0
            return finalColor;
        }
    };
}