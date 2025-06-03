/// FileName: Application.hpp
/// Date: 2025/05/11
/// Author: ChaomengOrion

#pragma once

#include "Pipeline.hpp"
#include "Scene.hpp"
#include <thread>

using namespace aries::render;
using namespace aries::scene;

namespace aries {
    class Application {
    private:
        sptr<Scene> scene;
        sptr<Pipeline> pipeline;
        
        // 渲染线程
        std::thread renderThread;
        // 渲染线程标志
        bool renderThreadRunning = false;

    public:
        void Init();
        void StartRenderThread();
        void OnUpdate();
    private:
        void LoadModel(const std::string& filename);
    };
}