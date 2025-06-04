#include "Scene.hpp"
#include "Pipeline.hpp"

using namespace aries::render;

namespace aries::scene {
    Scene::Scene(std::string sceneName, sptr<Pipeline> pipeline)
        : name(std::move(sceneName))
        , pipeline(std::move(pipeline)) {}

    // 添加模型到场景
    void Scene::AddModel(sptr<Model> model) {
        if (!model) {
            std::cerr << "[Scene] 无效的模型指针，无法添加。" << std::endl;
            return;
        }
        if (models.find(model->name) != models.end()) {
            std::cerr << "[Scene] 模型 " << model->name << " 已存在，无法重复添加。" << std::endl;
            return;
        }
        std::cout << "[Scene] 添加模型：" << model->name << std::endl;
        models[model->name] = model;
        for (const auto& shape : model->shapes) {
            pipeline->AddShape(shape);
        }
    }

    void Scene::ClearObjects() { // TODO
        std::cout << "[Scene] 清空场景中的所有模型和形状。" << std::endl;
        //pipeline->shapeListMutex.lock();
        models.clear();
        pipeline->shapeList.clear(); // 清空管线中的形状列表
        //pipeline->shapeListMutex.unlock();
    }

    // 获取场景中的模型
    sptr<Model> Scene::GetModel(const std::string& name) {
        return models.at(name);
    }

    // 设置相机
    void Scene::SetCamera(sptr<Camera> cam) {
        camera = cam;
    }

    // 获取相机
    sptr<Camera>& Scene::GetCamera() {
        return camera;
    }

    #pragma region DEBUG
    void Scene::SetTestCamera() {
        std::cout << "[Scene] 设置测试相机" << std::endl;
        camera = std::make_shared<Camera>();
        camera->Position = Vector4f(0, 0, 10, 1);
        camera->Direction = Vector3f(0, 0, -1).normalized();
        camera->Up = Vector3f(0, 1, 0).normalized();
        camera->Fov = 60.f;
        camera->Near = 0.1f;
        camera->Far = 60.f;
        auto config = SharedConfigManager::GetConfig();
        camera->AspectRatio = (float)config->framebuffer_width / (float)config->framebuffer_height;
    }

    void Scene::SetTestLight() {
        std::cout << "[Scene] 设置测试光源" << std::endl;
        mainLight = std::make_shared<Light>();
        mainLight->position = Vector4f(0, 10, 10, 1);
        mainLight->color = Vector3f(1, 1, 1); // 白色光
        mainLight->intensity = 1.0f; // 光强度
        mainLight->direction = Vector3f(0, -1, -1).normalized(); // 光源方向
    }
    #pragma endregion
}