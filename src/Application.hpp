/// FileName: Application.hpp
/// Date: 2025/05/11
/// Author: ChaomengOrion

#pragma once

#include "Pipeline.hpp"
#include "Scene.hpp"
#include <thread>
#include "Render/Materials/M_ShadowedBlinnPhongMaterial.hpp"

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

    private:
        // 新增的UI方法
        void ShowSceneManagerWindow(bool* p_open);
        void ShowModelLoaderWindow(bool* p_open);
        void ShowConfigWindow(bool* p_open, sptr<Camera> camera);
        void ShowMaterialEditor(sptr<Shape> shape);
        
        // 材质预设方法
        void ApplyDefaultMaterial(ShadowedBlinnPhongMaterial::property_t& prop);
        void ApplyMetallicMaterial(ShadowedBlinnPhongMaterial::property_t& prop);
        void ApplyPlasticMaterial(ShadowedBlinnPhongMaterial::property_t& prop);
        void ApplyRubberMaterial(ShadowedBlinnPhongMaterial::property_t& prop);
        void ApplyJadeMaterial(ShadowedBlinnPhongMaterial::property_t& prop);
        
    public:
        void Init();
        void SetupImGuiStyle();
        // void StartRenderThread();
        void OnUpdate(SharedConfig& config);
        void LoadModel(const std::string& filename);
        void ProcessMouseInput(double xpos, double ypos);
        void ProcessMouseButton(int button, int action);
        void ProcessMouseScroll(double yoffset);
    };
}