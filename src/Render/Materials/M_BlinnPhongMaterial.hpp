#pragma once

#include "Material.hpp"

namespace aries::material {
    class BlinnPhongMaterial : public MaterialBase<BlinnPhongShader> {
    public:
        BlinnPhongMaterial(std::string name, sptr<Texture> tex) : MaterialBase(name) {
            property.texture = std::move(tex);
            shader = std::make_shared<BlinnPhongShader>();
        }
    };
}