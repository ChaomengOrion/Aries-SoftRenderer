#pragma once

#include "Material.hpp"

namespace aries::material {
    class PreviewMaterial : public MaterialBase<PreviewShader> {
    public:
        PreviewMaterial(std::string name) : MaterialBase(name) { }
    };
}