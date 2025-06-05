/// FileName: S_BlinnPhongShader.hpp
/// Date: 2025/05/31
/// Author: ChaomengOrion

#pragma once

#include "Shader.hpp"
#include "../Texture.hpp"
#include "../../Scene.hpp"

namespace aries::shader {

    template<> // 特化v2f类型
    struct v2f<class BlinnPhongShader> {
        Vector4f screenPos;   // 屏幕空间坐标（viewport * MVP * position）//* 必须包含
        Vector4f viewPos;    // 视图空间坐标（V * M * pos）
        Vector3f normal;     // 世界/视图空间法线
        Vector2f uv;         // 纹理坐标
    };

    
    template<> // 特化ShaderProperty类型
    struct ShaderProperty<class BlinnPhongShader> {
        sptr<Texture> texture; // 纹理

        Vector3f ambient = Vector3f(1.f, 1.f, 1.f); // 环境光
        float ambientIntensity = 0.43f; // 环境光强度

        Vector3f diffuse = Vector3f(1.f, 1.f, 1.f); // 漫反射光
        float diffuseIntensity = 0.47f; // 漫反射光强度

        Vector3f specular = Vector3f(1.f, 1.f, 1.f); // 镜面反射光
        float specularIntensity = 0.1f; // 镜面反射光强度

        float shininess = 64.f; // 高光指数，用于控制高光的锐利程度
    };

    class BlinnPhongShader : public ShaderBase<BlinnPhongShader> {
        inline static Light* light; // 主光源缓存

    public:
        // CRTP实现 - 编译器知道确切类型，可内联优化
        constexpr static ShaderType GetTypeImpl() {
            return ShaderType::BlinnPhong; // 返回着色器类型
        }

        inline static void BeforeShaderImpl(const payload_t& payload) {
            light = payload.scene->mainLight.get(); // 缓存主光源
        }
        
        inline static v2f_t VertexShaderImpl(const a2v& data, const Matrixs& matrixs, const property_t&) {
            v2f_t v2fData;
            v2fData.screenPos = matrixs.mat_mvp * data.position; // 计算裁剪空间坐标
            v2fData.viewPos = matrixs.mat_view * data.position; // 计算视图空间坐标
            v2fData.normal = (matrixs.mat_view * Vector4f(data.normal.x(), data.normal.y(), data.normal.z(), 0.0f)).template head<3>().normalized(); // 法线转换
            v2fData.uv = data.uv; // 直接传递纹理坐标
            return v2fData;
        }

        inline static Vector3f FragmentShaderImpl(const v2f_t& data, const Matrixs& matrixs, const property_t& property) {
            // 获取纹理坐标
            Vector2f uv = data.uv;

            // 这里用裸指针加速，避免 shared_ptr 原子操作的开销，经过测试，性能提升明显（2025-6-1测试 60fps -> 100fps 13万顶点）
        
            Texture* texture = property.texture.get();

            Vector3f color = texture ? texture->Sample(uv.x(), uv.y()) : Vector3f(1.f, 1.f, 1.f);

            if (!light || !texture) [[unlikely]] {
                return color; // 如果没有光源，直接返回颜色
            }

            // Blinn-Phong 着色模型
            Vector3f N = data.normal.normalized(); // 法线，相机空间
            Vector3f L = -(matrixs.mat_view * Vector4f(light->direction.x(), light->direction.y(), light->direction.z(), 0)).head<3>().normalized(); // 光照方向
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