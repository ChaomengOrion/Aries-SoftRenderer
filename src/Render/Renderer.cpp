#include "Renderer.hpp"
#include "Shaders/ShaderRegister.hpp"
#include <numbers>

namespace aries::render {
    inline constexpr float ToRadian(float angle) {
        return (angle / 180) * std::numbers::pi_v<float>;
    }

    Renderer::Renderer(sptr<Raster> raster, int w, int h)
        : _width(w) , _height(h) {
        if (w <= 0 || h <= 0) {
            throw std::invalid_argument("Renderer width and height must be positive integers.");
        }
        std::cout << "[Renderer]" << "新建渲染器：" << w << 'x' << h << '\n';
        m_raster = std::move(raster);
        _zBuffer.resize(w * h);

        _viewport <<    _width / 2., 0, 0, _width / 2., 
                        0, _height / 2., 0, _height / 2., 
                        0, 0, 1, 0, 
                        0, 0, 0, 1;
    }

    void Renderer::Clear() {
        m_raster->ClearCurrentBuffer();
        std::fill(_zBuffer.begin(), _zBuffer.end(), /*std::numeric_limits<float>::infinity()*/ std::numeric_limits<float>::max()); // 或者使用最大值
    }

    Matrix4f Renderer::GetModelMatrix(const Model& o) {
        Matrix4f rX, rY, rZ;
        float radX, radY, radZ;
        Matrix4f scale;
        Matrix4f move;

        radX = ToRadian(o.rotation.x());
        radY = ToRadian(o.rotation.y());
        radZ = ToRadian(o.rotation.z());

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

        scale <<    o.scale.x(), 0, 0, 0, 
                    0, o.scale.y(), 0, 0, 
                    0, 0, o.scale.z(), 0, 
                    0, 0, 0, 1;

        move << 1, 0, 0, o.position.x(), 
                0, 1, 0, o.position.y(), 
                0, 0, 1, o.position.z(), 
                0, 0, 0, 1;

        return move * rZ * rX * rY * scale;
    }

    Matrix4f Renderer::GetViewMatrix(Camera& c) {
        c.ApplyEluaAngle(); // 应用欧拉角到摄像机方向和上向量

        // 将摄像机移动到原点，然后使用旋转矩阵的正交性让摄像机摆正
        Matrix4f move; // 移动矩阵
        Vector3f right; // 摄像机的x轴
        Matrix4f rotateT; // 旋转矩阵的转置矩阵

        move << 1, 0, 0, -c.Position.x(), 0, 1, 0, -c.Position.y(), 0, 0, 1, -c.Position.z(), 0, 0, 0,
            1;

        right = c.Direction.cross(c.Up);

        rotateT << right.x(), right.y(), right.z(), 0, c.Up.x(), c.Up.y(), c.Up.z(), 0,
            -c.Direction.x(), -c.Direction.y(), -c.Direction.z(), 0, 0, 0, 0, 1;

        return rotateT * move;
    }

    Matrix4f Renderer::GetClipMatrix(const Camera& c) {
        float radFov;
        Matrix4f frustum;

        radFov = ToRadian(c.Fov);

        frustum << 1 / (c.AspectRatio * tan(radFov / 2)), 0, 0, 0, 0, 1 / tan(radFov / 2), 0, 0, 0, 0,
            -(c.Far + c.Near) / (c.Far - c.Near), -(2 * c.Far * c.Near) / (c.Far - c.Near), 0, 0, -1, 0;

        return frustum;
    }

    void Renderer::RenderWithShader(ShaderType type, vector<sptr<Shape>>& shapeList, sptr<Scene>& scene, sptr<Camera>& cam, uint64_t& triangleCount) {
        ShaderDispatcher<RegisteredShaders>::Dispatch(*this, type, shapeList, scene, cam, triangleCount);
    }
}