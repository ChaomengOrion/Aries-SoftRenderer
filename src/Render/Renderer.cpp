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

    void Renderer::SetCameraAndScene(sptr<Camera> camera, sptr<Scene> scene) {
        m_camera = std::move(camera);
        m_scene = std::move(scene);
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

    Matrix4f Renderer::GetViewMatrix() {
        auto& c = *m_camera;

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

    Matrix4f Renderer::GetClipMatrix() {
        auto& c = *m_camera;
        float radFov;
        Matrix4f frustum;

        radFov = ToRadian(c.Fov);

        frustum << 1 / (c.AspectRatio * tan(radFov / 2)), 0, 0, 0, 0, 1 / tan(radFov / 2), 0, 0, 0, 0,
            -(c.Far + c.Near) / (c.Far - c.Near), -(2 * c.Far * c.Near) / (c.Far - c.Near), 0, 0, -1, 0;

        return frustum;
    }

    void Renderer::DrawLine3D(Vector3f start, Vector3f end, Vector3f color, bool ignoreDepthTest) {
        Vector4f startPos(start.x(), start.y(), start.z(), 1.0f);
        Vector4f endPos(end.x(), end.y(), end.z(), 1.0f);

        // 先只应用视图矩阵，检查点是否在摄像机前面
        Matrix4f viewMatrix = GetViewMatrix();
        Vector4f startView = viewMatrix * startPos;
        Vector4f endView = viewMatrix * endPos;
        
        // 检查点是否在摄像机后面 (-z 是朝向摄像机前方)
        const float NEAR_CLIP = 0.1f; //? 暂时不使用与摄像机的近平面一致的值
        
        // 两个点都在摄像机后面或太靠近，不绘制
        if (startView.z() > -NEAR_CLIP && endView.z() > -NEAR_CLIP) {
            return;
        }
        
        // 如果一个点在摄像机后面，需要裁剪到近平面
        if (startView.z() > -NEAR_CLIP) {
            // 计算裁剪因子
            float t = (-NEAR_CLIP - startView.z()) / (endView.z() - startView.z());
            startView = startView + t * (endView - startView);
        }
        else if (endView.z() > -NEAR_CLIP) {
            // 计算裁剪因子
            float t = (-NEAR_CLIP - endView.z()) / (startView.z() - endView.z());
            endView = endView + t * (startView - endView);
        }
        
        // 应用投影和视口变换
        Matrix4f projViewport = _viewport * GetClipMatrix();
        Vector4f startVP = projViewport * startView;
        Vector4f endVP = projViewport * endView;

        // 透视除法
        Vector3f startScreen(startVP.x() / startVP.w(), startVP.y() / startVP.w(), startVP.z() / startVP.w());
        Vector3f endScreen(endVP.x() / endVP.w(), endVP.y() / endVP.w(), endVP.z() / endVP.w());

        // Bresenham算法绘制线
        int x0 = static_cast<int>(startScreen.x());
        int y0 = static_cast<int>(startScreen.y());
        int x1 = static_cast<int>(endScreen.x());
        int y1 = static_cast<int>(endScreen.y());

        bool steep = false, swapStartEnd = false;
        if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
            std::swap(x0, y0);
            std::swap(x1, y1);
            steep = true;
        }

        if (x0 > x1) {
            std::swap(x0, x1);
            std::swap(y0, y1);
            swapStartEnd = true;
        }

        int dx = x1 - x0;
        int dy = y1 - y0;
        int derror2 = std::abs(dy) * 2;
        int error2 = 0;
        int y = y0;

        for (int x = x0; x <= x1; x++) {
            int drawX = steep ? y : x;
            int drawY = steep ? x : y;

            if (drawX >= 0 && drawX < _width && drawY >= 0 && drawY < _height) {
                //* 插值
                float t = (float)(x - x0) / std::max(1, dx);
                if (swapStartEnd) t = 1 - t;
                
                float depth = ((1 - t) * startScreen.z() + t * endScreen.z());
                
                // 应用深度测试
                int idx = GetPixelIndex(drawX, drawY);
                if (ignoreDepthTest || _zBuffer[idx] > depth) {
                    if (!ignoreDepthTest) _zBuffer[idx] = depth;
                    SetPixelColor(drawX, drawY, color);
                }
            }

            error2 += derror2;
            if (error2 > dx) {
                y += (y1 > y0 ? 1 : -1);
                error2 -= dx * 2;
            }
        }
    }

    void Renderer::DrawCoordinateSystem(float axisLength, bool showGrid, float gridSize, int gridCount) {
        // 绘制主坐标轴
        DrawLine3D(Vector3f(0,0,0), Vector3f(axisLength,0,0), Vector3f(1,0,0)); // X轴 (红)
        DrawLine3D(Vector3f(0,0,0), Vector3f(0,axisLength,0), Vector3f(0,1,0)); // Y轴 (绿)
        DrawLine3D(Vector3f(0,0,0), Vector3f(0,0,axisLength), Vector3f(0,0,1)); // Z轴 (蓝)
        
        // 绘制坐标轴箭头
        const float arrowSize = 0.1f;
        
        // X轴箭头
        DrawLine3D(Vector3f(axisLength,0,0), Vector3f(axisLength-arrowSize,arrowSize/2,0), Vector3f(1,0,0));
        DrawLine3D(Vector3f(axisLength,0,0), Vector3f(axisLength-arrowSize,-arrowSize/2,0), Vector3f(1,0,0));
        
        // Y轴箭头
        DrawLine3D(Vector3f(0,axisLength,0), Vector3f(arrowSize/2,axisLength-arrowSize,0), Vector3f(0,1,0));
        DrawLine3D(Vector3f(0,axisLength,0), Vector3f(-arrowSize/2,axisLength-arrowSize,0), Vector3f(0,1,0));
        
        // Z轴箭头
        DrawLine3D(Vector3f(0,0,axisLength), Vector3f(0,arrowSize/2,axisLength-arrowSize), Vector3f(0,0,1));
        DrawLine3D(Vector3f(0,0,axisLength), Vector3f(0,-arrowSize/2,axisLength-arrowSize), Vector3f(0,0,1));
        
        // 绘制网格(如果启用)
        if (showGrid) {
            Vector3f gridColor(0.5f, 0.5f, 0.5f); // 灰色网格
            float extent = gridCount * gridSize;
            
            // XZ平面网格
            for (int i = -gridCount; i <= gridCount; i++) {
                float pos = i * gridSize;
                DrawLine3D(Vector3f(-extent, 0, pos), Vector3f(extent, 0, pos), gridColor, false);
                DrawLine3D(Vector3f(pos, 0, -extent), Vector3f(pos, 0, extent), gridColor, false);
            }
        }
    }

    void Renderer::RenderWithShader(ShaderType type, vector<sptr<Shape>>& shapeList, uint64_t& triangleCount) {
        ShaderDispatcher<RegisteredShaders>::Dispatch(*this, type, shapeList, triangleCount);
    }
}