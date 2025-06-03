#pragma once

#include "CommonHeader.hpp"

#include "Triangle.hpp"
#include "Shape.hpp"

namespace aries::model {
    class Model {
    public:
        Model(string name) : name(std::move(name)) { }

        Model(string name, vector<sptr<Shape>> shapes)
            : name(std::move(name)), shapes(std::move(shapes)) {
            for (size_t i = 0; i < this->shapes.size(); i++) {
                this->shapes[i]->model = this; // 设置每个形状的模型指针
            }
        }

        string name;

        Vector3f position = Vector3f(0.0f, 0.0f, 0.0f); // 模型位置
        Vector3f rotation = Vector3f(0.0f, 0.0f, 0.0f); // 模型旋转
        Vector3f scale = Vector3f(1.0f, 1.0f, 1.0f); // 模型缩放

        vector<sptr<Shape>> shapes; // 模型的形状列表
    };
}