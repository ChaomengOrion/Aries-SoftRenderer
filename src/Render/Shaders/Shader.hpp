/// FileName: Shader.hpp
/// Date: 2025/06/01
/// Author: ChaomengOrion

#pragma once

#include "../CommonHeader.hpp"

#include "../Camera.hpp"
#include "../Texture.hpp"

#include "ShaderTypes.hpp"

namespace aries::scene {
    class Scene; // 前向声明
}

namespace aries::shader {
    enum class ShaderType;

    template<typename ShaderT>
    struct ShaderProperty { };

    template<typename ShaderT>
    struct Payload {
        scene::Scene* scene; // 场景
        Camera* camera; // 相机
    };

    struct Matrixs {
        Matrix4f mat_model; // 模型->世界矩阵
        Matrix4f mat_view; // 模型->视图矩阵
        Matrix4f mat_mvp; // 模型->NDC矩阵
    };

    // a2v：顶点着色器输入
    struct a2v {
        Vector4f position; // 模型空间顶点 (x,y,z,1)
        Vector3f normal;   // 模型空间法线
        Vector2f uv;       // 纹理坐标
    };

    // v2f：顶点着色器输出 & 片元着色器输入，可自定义
    template<typename ShaderT>
    struct v2f {
        Vector4f screenPos;   // 屏幕空间坐标（viewport * MVP * position）//* 必须包含
    };

    // 着色器基类使用CRTP模式
    template<typename ShaderT>
    class ShaderBase {
    private:
        ShaderBase() = default;
        friend ShaderT; // 只有继承了ShaderBase<ShaderT>的ShaderT类可以访问私有构造函数

    public:
        using payload_t = Payload<ShaderT>; // 着色器输入数据类型
        using v2f_t = v2f<ShaderT>; // 顶点着色器输出 & 片元着色器输入
        using property_t = ShaderProperty<ShaderT>; // 着色器属性类型

        // 获取着色器类型 - 派生类必须实现
        constexpr static ShaderType GetType() {
            return ShaderT::GetTypeImpl();
        }

        // 默认的顶点着色器实现 - 派生类必须实现
        inline static v2f_t VertexShader(const a2v& data, const Matrixs& matrixs, const property_t& property) {
            return ShaderT::VertexShaderImpl(data, matrixs, property);
        }
        
        // 静态分派的片元着色器 - 派生类必须实现
        inline static Vector3f FragmentShader(const v2f_t& data, const Matrixs& matrixs, const property_t& property) {
            // 静态分派到实际的实现
            return ShaderT::FragmentShaderImpl(data, matrixs, property);
        }

        // 可选的片元着色器前处理，可以缓存内容
        inline static void BeforeShader(const payload_t& payload) {
            // 检查子类有没有实现
            if constexpr (requires { ShaderT::BeforeShaderImpl(payload); }) {
                ShaderT::BeforeShaderImpl(payload);
            }
        }
    };

    template<typename ShaderT>
    concept ShaderConcept = std::derived_from<ShaderT, ShaderBase<ShaderT>> &&
                            requires(ShaderT) {
                                { ShaderT::GetTypeImpl() } -> std::convertible_to<ShaderType>;
                                { ShaderT::VertexShaderImpl(std::declval<a2v>(), std::declval<Matrixs>(), std::declval<typename ShaderT::property_t>()) } -> std::convertible_to<typename ShaderT::v2f_t>;
                                { ShaderT::FragmentShaderImpl(std::declval<v2f<ShaderT>>(), std::declval<Matrixs>(), std::declval<typename ShaderT::property_t>()) } -> std::convertible_to<Vector3f>;
                            };
}