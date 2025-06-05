#pragma once

#include "Shader.hpp"
#include "../Texture.hpp"
#include "../../Scene.hpp"

namespace aries::shader {
    template<> // 特化v2f类型
    struct v2f<class ShadowedBlinnPhongShader> {
        Vector4f screenPos;   // 屏幕空间坐标（viewport * MVP * position）//* 必须包含
        Vector4f viewPos;     // 视图空间坐标（V * M * pos）
        Vector3f worldPos;    // 世界空间坐标（用于阴影计算）
        Vector3f normal;      // 世界/视图空间法线
        Vector2f uv;          // 纹理坐标
    };

    
    template<> // 特化ShaderProperty类型
    struct ShaderProperty<class ShadowedBlinnPhongShader> {
        sptr<Texture> texture; // 纹理

        Vector3f ambient = Vector3f(1.f, 1.f, 1.f); // 环境光
        float ambientIntensity = 0.43f; // 环境光强度

        Vector3f diffuse = Vector3f(1.f, 1.f, 1.f); // 漫反射光
        float diffuseIntensity = 0.47f; // 漫反射光强度

        Vector3f specular = Vector3f(1.f, 1.f, 1.f); // 镜面反射光
        float specularIntensity = 0.1f; // 镜面反射光强度

        float shininess = 64.f; // 高光指数，用于控制高光的锐利程度
        
        // 阴影相关参数
        float shadowBias = 0.001f;    // 阴影偏移，避免阴影痤疮
        float shadowIntensity = 0.8f; // 阴影强度（0-1）
        int pcfSamples = 3;           // PCF 采样数量（1=硬阴影，3/5=软阴影）

        // 距离衰减参数
        float shadowDistanceAttenuation = 20.0f;  // 距离衰减系数，值越大衰减越快
        float shadowMinIntensity = 0.1f;          // 最小阴影强度
    };

    class ShadowedBlinnPhongShader : public ShaderBase<ShadowedBlinnPhongShader> {
        inline static DirectionalShadow* shadowSystem; // 阴影系统缓存

    public:
        // CRTP实现 - 编译器知道确切类型，可内联优化
        constexpr static ShaderType GetTypeImpl() {
            return ShaderType::ShadowedBlinnPhong; // 返回着色器类型
        }

        inline static void BeforeShaderImpl(const payload_t& p) {
            shadowSystem = p.scene->directionalShadow.get(); // 缓存阴影系统
        }
        
        inline static v2f_t VertexShaderImpl(const a2v& data, const Matrixs& matrixs, const property_t&) {
            v2f_t v2fData;
            v2fData.screenPos = matrixs.mat_mvp * data.position; // 计算裁剪空间坐标
            v2fData.viewPos = matrixs.mat_view * data.position; // 计算视图空间坐标
            
            // 计算世界空间坐标（用于阴影计算）
            v2fData.worldPos = (matrixs.mat_model * data.position).head<3>();
            
            v2fData.normal = (matrixs.mat_view * Vector4f(data.normal.x(), data.normal.y(), data.normal.z(), 0.0f)).template head<3>().normalized(); // 法线转换
            v2fData.uv = data.uv; // 直接传递纹理坐标
            return v2fData;
        }

        inline static Vector3f FragmentShaderImpl(const v2f_t& data, const Matrixs& matrixs, const property_t& property) {
            // 获取纹理坐标
            Vector2f uv = data.uv;
            Texture* texture = property.texture.get();
            Vector3f color = texture ? texture->Sample(uv.x(), uv.y()) : Vector3f(1.f, 1.f, 1.f);

            if (!shadowSystem) [[unlikely]] {
                return color;
            }

            auto lightDir = shadowSystem->GetLightDirection();

            // Blinn-Phong 着色模型
            Vector3f N = data.normal.normalized();
            Vector3f L = -(matrixs.mat_view * Vector4f(lightDir.x(), lightDir.y(), lightDir.z(), 0)).head<3>().normalized();
            Vector3f V = -data.viewPos.head<3>().normalized();
            Vector3f H = (L + V).normalized();
            float diff = std::max(N.dot(L), 0.0f);
            float spec = std::pow(std::max(N.dot(H), 0.0f), property.shininess);

            // 计算基础光照颜色
            Vector3f ambient = property.ambient * property.ambientIntensity;
            Vector3f diffuse = property.diffuse * property.diffuseIntensity * diff;
            Vector3f specular = property.specular * property.specularIntensity * spec;

            // 获取阴影信息
            auto shadowInfo = GetShadowInfo(property, data);
            
            // 分别应用不同的阴影因子
            // 1. 直接光照（漫反射+镜面反射）使用常数阴影强度
            Vector3f directLighting = diffuse + specular;
            Vector3f shadedDirectLighting = directLighting * (1.0f - shadowInfo.constantShadowFactor * property.shadowIntensity);
            
            // 2. 环境光使用距离衰减的阴影强度
            Vector3f shadedAmbient = ambient * (1.0f - shadowInfo.distanceAttenuatedShadowFactor * property.shadowIntensity);
            
            Vector3f finalColor = color.cwiseProduct(shadedAmbient + shadedDirectLighting);
            finalColor = finalColor.cwiseMin(Vector3f(1.0f, 1.0f, 1.0f));
            return finalColor;
        }

    private:
        // 阴影信息结构
        struct ShadowInfo {
            float constantShadowFactor;          // 常数阴影因子（用于直接光照）
            float distanceAttenuatedShadowFactor; // 距离衰减阴影因子（用于环境光）
        };

        // 获取分层阴影信息
        inline static ShadowInfo GetShadowInfo(const property_t& property, const v2f_t& data) {
            if (!shadowSystem) {
                return {0.0f, 0.0f};
            }

            // 使用增强的阴影采样
            aries::shadow::ShadowSampleResult shadowResult;
            if (property.pcfSamples <= 1) {
                shadowResult = shadowSystem->SampleShadowWithDistance(data.worldPos, property.shadowBias);
            } else {
                shadowResult = shadowSystem->SampleShadowPCFWithDistance(data.worldPos, property.pcfSamples, property.shadowBias);
            }

            if (shadowResult.shadowFactor < 0.001f) {
                return {0.0f, 0.0f}; // 没有阴影
            }

            // 1. 常数阴影因子（用于直接光照）
            float constantShadowFactor = shadowResult.shadowFactor;

            // 2. 距离衰减阴影因子（用于环境光）
            float distanceAttenuation = CalculateDistanceAttenuation(
                shadowResult.occluderDistance, 
                property.shadowDistanceAttenuation
            );
            
            float distanceAttenuatedShadowFactor = shadowResult.shadowFactor * distanceAttenuation;
            
            // 确保最小阴影强度（只对环境光）
            if (distanceAttenuatedShadowFactor > 0.001f) {
                distanceAttenuatedShadowFactor = std::max(distanceAttenuatedShadowFactor, property.shadowMinIntensity);
            }

            return {constantShadowFactor, distanceAttenuatedShadowFactor};
        }

        // 距离衰减计算函数
        inline static float CalculateDistanceAttenuation(float occluderDistance, float attenuationFactor) {
            constexpr float maxDistance = 2.f;

            if (occluderDistance <= 0.0f) return 1.0f;
            if (occluderDistance >= maxDistance) return 0.0f;

            // 指数衰减函数
            float normalizedDistance = occluderDistance / maxDistance;
            float attenuation = std::exp(-attenuationFactor * normalizedDistance);
            
            return std::clamp(attenuation, 0.0f, 1.0f);
        }
    };
}