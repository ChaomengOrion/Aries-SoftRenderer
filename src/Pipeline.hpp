/// FileName: Pipeline.hpp
/// Date: 2025/05/12
/// Author: ChaomengOrion

#pragma once

#include "Render/CommonHeader.hpp"

#include "Render/Camera.hpp"
#include "Render/Raster.hpp"
#include "Render/Renderer.hpp"
#include "Render/Shape.hpp"

#include <shared_mutex>

using namespace aries::scene;

namespace aries::render {
    class Pipeline {
    public:
        vector<std::weak_ptr<Shape>> shapeList; // 对象列表，弱引用
        sptr<Raster> raster;
        sptr<Renderer> renderer;

        //std::shared_mutex shapeListMutex; // 保护shapeList的互斥锁，避免渲染线程和主线程冲突
        bool showCoordinateSystem = true; // 是否显示坐标系

        uint64_t triangleCount = 0; // 三角形计数
        float frameTime = 0.0f; // 帧时间

        Pipeline();

        void InitFrameSize();

        void Render(sptr<Scene> scene, sptr<Camera> cam);

        void Draw();

        void AddShape(std::weak_ptr<Shape> obj);

        void CleanUpUnusedShapes();
    };
}