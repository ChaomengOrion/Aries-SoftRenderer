/// FileName: Shape.hpp
/// Date: 2025/06/01
/// Author: ChaomengOrion

#pragma once
#include "CommonHeader.hpp"

#include "Triangle.hpp"

namespace aries::material {
    class IMaterial;
}

namespace aries::model {
    class Model; // 前向声明

    class Shape {
    public:
        Shape() = default;

        string name; // 形状名称

        Model* model; // 属于的模型

        vector<Triangle> mesh;

        sptr<material::IMaterial> material; // 材质球
    };
}