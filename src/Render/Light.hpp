#pragma once
#include "CommonHeader.hpp"

#include "Camera.hpp"

struct Light
{
    Vector3f direction;
    Vector3f color;
    float intensity;
    Vector4f position;

    int shadowMapWidth;
    int shadowMapHeight;
    sptr<Camera> virtualCamera;

    std::vector<float> shadowMap;
};