/// FileName: ShaderRegister.hpp
/// Date: 2025/06/03
/// Author: ChaomengOrion

#pragma once

#include "Shader.hpp"
#include "S_PreviewShader.hpp"
#include "S_BlinnPhongShader.hpp"
#include "S_ShadowedBlinnPhongShader.hpp"

namespace aries::shader {
    // 类型列表 - 用于存储所有着色器类型
    template<ShaderConcept... Ts> struct TypeList {};

    // 着色器类型注册表
    using RegisteredShaders = TypeList<
        ShadowedBlinnPhongShader,
        BlinnPhongShader,
        PreviewShader
    >;
}