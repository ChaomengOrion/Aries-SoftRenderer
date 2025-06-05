#pragma once

#include "Material.hpp"

namespace aries::material {
    class ShadowedBlinnPhongMaterial : public MaterialBase<ShadowedBlinnPhongShader> {
    public:
        ShadowedBlinnPhongMaterial(std::string name, sptr<Texture> tex) : MaterialBase(name) {
            property.texture = std::move(tex);
        }
    };
}