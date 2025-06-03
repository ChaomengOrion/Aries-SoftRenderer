/// FileName: Pipeline.cpp
/// Date: 2025/05/12
/// Author: ChaomengOrion

#include "Pipeline.hpp"
#include "SharedConfig.hpp"

#include <chrono>

namespace aries::render {

    Pipeline::Pipeline() = default;

    void Pipeline::InitFrameSize() {
        auto config = SharedConfigManager::GetConfig();
        int w = config->framebuffer_width;
        int h = config->framebuffer_height;
        std::cout << "[Pipeline]" << "帧缓冲区被设置为" << w << 'x' << h << '\n';
        raster = std::make_shared<Raster>();
        raster->InitBuffers(w, h);
        renderer = std::make_shared<Renderer>(raster, w, h);
    }

    void Pipeline::Draw(sptr<Scene>& scene, sptr<Camera>& cam) {
        static auto lastFrameTime = std::chrono::system_clock::now();
        static auto currentFrameTime = std::chrono::system_clock::now();

        currentFrameTime = std::chrono::system_clock::now();
        std::chrono::duration<float> elapsed = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;
        frameTime = elapsed.count(); // 计算帧率

        //* 清空
        renderer->Clear();

        shapeListMutex.lock_shared();
        
        //* 把形状按着色器类型分组
        std::unordered_map<ShaderType, vector<sptr<Shape>>> shapeGroups;
        for (auto& wk : shapeList) {
            if (auto shape = wk.lock()) {
                if (shape->material) {
                    shapeGroups[shape->material->GetShaderType()].emplace_back(std::move(shape));
                }
            }
        }

        // 统计三角形数量
        uint64_t tempCnt = 0;

        //* 送入渲染器
        for (auto [shaderType, shapes] : shapeGroups) {
            //auto ret = renderer->VertexShaderWith<BlinnPhongShader>(shapes, scene, cam);
            renderer->RenderWithShader(shaderType, shapes, scene, cam, tempCnt);
        }

        triangleCount = tempCnt; // 更新三角形计数

        shapeListMutex.unlock_shared();

        //* 交换CPU缓冲区
        raster->SwapBuffers();
    }

    void Pipeline::OnUpdate() {
        // 4. 提交并绘制
        raster->UploadToGPU();
        raster->Draw();
    }

    void Pipeline::AddShape(std::weak_ptr<Shape> obj) {
        if (auto shape = obj.lock()) {
            shapeListMutex.lock();
            shapeList.push_back(shape);
            shapeListMutex.unlock();
            std::cout << "[Pipeline] 添加形状：" << shape->name << "，三角形数：" << shape->mesh.size()
                    << '\n';
        } else {
            std::cerr << "[Pipeline] 添加形状失败：形状已被销毁或无效。\n";
        }
    }

    void Pipeline::CleanUpUnusedShapes() {
        shapeListMutex.lock();
        shapeList.erase(
            std::remove_if(shapeList.begin(),
                        shapeList.end(),
                        [](const std::weak_ptr<Shape>& shape) { return shape.expired(); }),
            shapeList.end());
        shapeListMutex.unlock();
    }
}