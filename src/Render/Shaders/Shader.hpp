/// FileName: Shader.hpp
/// Date: 2025/06/01
/// Author: ChaomengOrion

#pragma once

#include "../CommonHeader.hpp"

#include "../Camera.hpp"
#include "../Texture.hpp"

namespace aries::scene {
    class Scene; // 前向声明
}

namespace aries::shader {
    enum class ShaderType {
        BlinnPhong,
        Texture,
        PBR,
        // 其他材质类型...
    };

    template<typename ShaderT>
    struct ShaderProperty { };

    template<typename ShaderT>
    struct VertexPaylod {
        Camera* camera; // 相机
        Matrix4f view; // 模型矩阵
        Matrix4f mvp; // 模型视图投影矩阵
        ShaderProperty<ShaderT>* property; // 着色器属性
    };

    template<typename ShaderT>
    struct FragmentPaylod {
        scene::Scene* scene; // 场景
        Camera* camera; // 相机
        Matrix4f view; // 模型矩阵
        ShaderProperty<ShaderT>* property; // 着色器属性
    };

    // a2v：顶点着色器输入
    struct a2v {
        Vector4f position; // 模型空间顶点 (x,y,z,1)
        Vector3f normal;   // 模型空间法线
        Vector2f uv;       // 纹理坐标
    };

    // v2f：顶点着色器输出 & 片元着色器输入
    template<typename ShaderT>
    struct v2f {
        Vector4f viewport;   // 屏幕空间坐标（viewport * MVP * position）
        Vector4f viewPos;    // 视图空间坐标（V * M * pos）
        Vector3f normal;     // 世界/视图空间法线
        Vector2f uv;         // 纹理坐标
    };

    // 着色器基类使用CRTP模式
    template<typename ShaderT>
    class ShaderBase {
    private:
        ShaderBase() = default;
        friend ShaderT; // 只有继承了ShaderBase<ShaderT>的ShaderT类可以访问私有构造函数

    public:
        using FragmentPaylod_t = FragmentPaylod<ShaderT>; // 片元着色器输入
        using VertexPaylod_t = VertexPaylod<ShaderT>; // 顶点着色器输入
        using v2f_t = v2f<ShaderT>; // 顶点着色器输出 & 片元着色器输入

        // 获取着色器类型 - 派生类必须实现
        constexpr static ShaderType GetType() {
            return ShaderT::GetTypeImpl();
        }

        // 默认的顶点着色器实现
        inline static v2f_t VertexShader(const VertexPaylod_t& paylod, const a2v& data) {
            v2f_t v2fData;
            v2fData.viewport = paylod.mvp * data.position; // 计算裁剪空间坐标
            v2fData.viewPos = paylod.view * data.position; // 计算视图空间坐标
            v2fData.normal = (paylod.view * Vector4f(data.normal.x(), data.normal.y(), data.normal.z(), 0.0f)).template head<3>().normalized(); // 法线转换
            v2fData.uv = data.uv; // 直接传递纹理坐标
            return v2fData;
        }
        
        // 静态分派的片元着色器 - 派生类必须实现
        inline static Vector3f FragmentShader(const FragmentPaylod_t& paylod, const v2f_t& data) {
            // 静态分派到实际的实现
            return ShaderT::FragmentShaderImpl(paylod, data);
        }
    };

    template<typename ShaderT>
    concept ShaderConcept = std::derived_from<ShaderT, ShaderBase<ShaderT>> &&
                            requires(ShaderT) {
                                { ShaderT::GetTypeImpl() } -> std::convertible_to<ShaderType>;
                                { ShaderT::FragmentShaderImpl(std::declval<FragmentPaylod<ShaderT>>(), std::declval<v2f<ShaderT>>()) } -> std::convertible_to<Vector3f>;
                            };

    template<>
    struct ShaderProperty<class BlinnPhongShader> {
        sptr<Texture> texture; // 纹理

        Vector3f ambient = Vector3f(1.f, 1.f, 1.f); // 环境光
        float ambientIntensity = 0.1f; // 环境光强度

        Vector3f diffuse = Vector3f(1.f, 1.f, 1.f); // 漫反射光
        float diffuseIntensity = 0.889f; // 漫反射光强度

        Vector3f specular = Vector3f(1.0f, 0.8f, 0.5f); // 镜面反射光
        float specularIntensity = 0.288f; // 镜面反射光强度

        float shininess = 8.f; // 高光指数，用于控制高光的锐利程度
    };
}