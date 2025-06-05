#pragma once

#include "Render/Camera.hpp"
#include "Render/Model.hpp"
#include "Render/Light.hpp"
#include "SharedConfig.hpp"
#include "Render/Shadow/DirectionalShadow.hpp"

using namespace aries::model;
using namespace aries::shadow;

namespace aries::render {
    class Pipeline; // 前向声明
}

namespace aries::scene {

    class Scene {
    public:
        Scene(std::string sceneName, sptr<render::Pipeline> pipeline);

        // 添加模型到场景
        void AddModel(sptr<Model> model);

        void ClearObjects();

        // 获取场景中的模型
        sptr<Model> GetModel(const std::string& name);

        // 设置相机
        void SetCamera(sptr<Camera> cam);

        // 获取相机
        sptr<Camera>& GetCamera();

        std::string name; // 场景名称

    #pragma region DEBUG
        void SetTestCamera();

        void SetTestLight();
    #pragma endregion

        sptr<Light> mainLight; // 场景主光源
        std::unique_ptr<DirectionalShadow> directionalShadow; // 平行光阴影映射
        std::unordered_map<string, sptr<Model>> models; // 场景中的模型列表
        sptr<Camera> camera; // 场景的相机
        sptr<render::Pipeline> pipeline; // 渲染管线
    };
}