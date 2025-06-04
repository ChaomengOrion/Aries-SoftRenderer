#pragma once

#include "../Shaders/ShaderRegister.hpp"

using namespace aries::shader;

namespace aries::material {
    class IMaterial { 
    public:
        virtual ShaderType GetShaderType() const = 0; // 获取着色器类型
        virtual ~IMaterial() = default;
    };

    template<ShaderConcept ShaderT>
    class MaterialBase : public IMaterial {
    protected:
        MaterialBase(string name) : name(std::move(name)) {}

    public:
        using shader_t = ShaderT; // 着色器类型

        string name; // 材质名称

        ShaderProperty<shader_t> property; // 着色器属性

        //// sptr<shader_t> shader; // 着色器

        ShaderType GetShaderType() const override {
            return ShaderBase<ShaderT>::GetType();
        }

        /* shader_t& GetShader() const {
            return *shader;
        }*/

        virtual ~MaterialBase() = default;
    };
}