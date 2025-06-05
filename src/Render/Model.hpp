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

        Matrix4f GetModelMatrix() const {
            Matrix4f rX, rY, rZ;
            float radX, radY, radZ;
            Matrix4f mat_scale;
            Matrix4f mat_move;

            radX = ToRadian(rotation.x());
            radY = ToRadian(rotation.y());
            radZ = ToRadian(rotation.z());

            rX <<   1, 0, 0, 0, 
                    0, cos(radX), -sin(radX), 0, 
                    0, sin(radX), cos(radX), 0, 
                    0, 0, 0, 1;

            rY <<   cos(radY), 0, sin(radY), 0, 
                    0, 1, 0, 0, 
                    -sin(radY), 0, cos(radY), 0, 
                    0, 0, 0, 1;

            rZ <<   cos(radZ), -sin(radZ), 0, 0, 
                    sin(radZ), cos(radZ), 0, 0, 
                    0, 0, 1, 0, 
                    0, 0, 0, 1;

            mat_scale <<    scale.x(), 0, 0, 0, 
                            0, scale.y(), 0, 0, 
                            0, 0, scale.z(), 0, 
                            0, 0, 0, 1;

            mat_move << 1, 0, 0, position.x(), 
                        0, 1, 0, position.y(), 
                        0, 0, 1, position.z(), 
                        0, 0, 0, 1;

            return mat_move * rZ * rX * rY * mat_scale;
        }

        string name;

        Vector3f position = Vector3f(0.0f, 0.0f, 0.0f); // 模型位置
        Vector3f rotation = Vector3f(0.0f, 0.0f, 0.0f); // 模型旋转
        Vector3f scale = Vector3f(1.0f, 1.0f, 1.0f); // 模型缩放

        vector<sptr<Shape>> shapes; // 模型的形状列表
    };
}