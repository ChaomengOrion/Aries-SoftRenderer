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

    void Scene::RemoveModel(const std::string& name) {
        auto it = models.find(name);
        if (it != models.end()) {
            std::cout << "[Scene] 移除模型：" << name << std::endl;
            models.erase(it);
            pipeline->CleanUpUnusedShapes(); // 清理管线中未使用的形状
        } else {
            std::cerr << "[Scene] 模型 " << name << " 不存在，无法移除。" << std::endl;
        }
    }

    void Scene::CopyModel(const std::string& name, const std::string& newName) {
        auto it = models.find(name);
        if (it != models.end()) {
            auto model = it->second;
            auto newModel = std::make_shared<Model>(newName);
            // 深克隆形状
            for (const auto& shape : model->shapes) {
                auto newShape = std::make_shared<Shape>(); // 深拷贝形状
                newShape->name = shape->name + "_copy"; // 修改新形状的名称
                newShape->mesh = shape->mesh; // 直接引用原始网格数据
                // TODO: 深拷贝材质
                newShape->material = shape->material; // 直接引用原始材质
                newShape->model = newModel.get(); // 设置新模型的引用
                newModel->shapes.push_back(newShape);
            }
            newModel->position = model->position + Vector3f(1.f, 0.f, 0.f); // 避免与原模型重叠
            newModel->rotation = model->rotation; // 复制旋转
            newModel->scale = model->scale; // 复制缩放
            AddModel(newModel); // 添加到场景
            std::cout << "[Scene] 复制模型：" << name << " 为 " << newName << std::endl;
        } else {
            std::cerr << "[Scene] 模型 " << name << " 不存在，无法复制。" << std::endl;
        }
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
        
        std::cout << "[Scene] 新建平行光" << std::endl;
        directionalShadow = std::make_unique<DirectionalShadow>(Vector3f(0, -1, -1).normalized(), 2048); // 创建一个1024x1024的阴影映射
    }
    #pragma endregion
}