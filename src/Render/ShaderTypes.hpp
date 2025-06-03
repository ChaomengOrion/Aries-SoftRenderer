#include "S_BlinnPhongShader.hpp"
#include "Shader.hpp"

enum class ShaderType {
    NullShader,          // 空着色器
    PreviewShader,       // 预览着色器
    TextureShader,       // 纹理着色器
    BlinnPhongShader,    // 布林-冯氏着色器
    NormalTestShader,    // 法线测试着色器
};