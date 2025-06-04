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

        // 鼠标控制
        bool mouseMiddlePressed = false;
        float lastMouseX = 0.0f;
        float lastMouseY = 0.0f;
        float mouseSensitivity = 0.1f; // 鼠标灵敏度

    public:
        void Init();
        // void StartRenderThread();
        void OnUpdate(SharedConfig& config);
        void LoadModel(const std::string& filename);
        void ProcessMouseInput(double xpos, double ypos);
        void ProcessMouseButton(int button, int action);
        void ProcessMouseScroll(double yoffset);
    };
}