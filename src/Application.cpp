/// FileName: Application.cpp
/// Date: 2025/05/11
/// Author: ChaomengOrion

#include "Application.hpp"

#include "SharedConfig.hpp"
#include "ObjLoader.hpp"

#include "ImGuiFileDialog.h"

namespace aries {
    void Application::Init() {
        SetupImGuiStyle(); // 设置ImGui样式

        pipeline = std::make_shared<Pipeline>();
        scene = std::make_shared<Scene>("Test Scene", pipeline);

        pipeline->InitFrameSize();
        scene->SetTestCamera(); // 设置测试相机
        scene->SetTestLight(); // 设置测试光源

        
        /*if (!renderThreadRunning) {
            StartRenderThread(); // 启动渲染线程
        }*/
    }

    /*void Application::StartRenderThread() {
        //! temp
        return; // 暂时禁用渲染线程
        renderThreadRunning = true;
        renderThread = std::thread([this]() {
            while (renderThreadRunning) {
                pipeline->Draw(scene, scene->GetCamera());
            }
        });
    }*/

    void Application::SetupImGuiStyle() {
        ImGuiStyle& style = ImGui::GetStyle();
        
        // 设置暗色主题
        ImGui::StyleColorsDark();
        
        // 自定义颜色
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.95f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.2f, 0.54f);
        colors[ImGuiCol_Button] = ImVec4(0.4f, 0.4f, 0.4f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.6f, 0.6f, 0.6f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.8f, 0.8f, 0.8f, 1.00f);
        
        // 设置圆角
        style.WindowRounding = 5.0f;
        style.FrameRounding = 3.0f;
        style.ScrollbarRounding = 3.0f;
        style.GrabRounding = 3.0f;
    }

    void Application::OnUpdate(SharedConfig& config) {
        const float deltaTime = 1.0f / config.io.Framerate;

        static bool show_loader_window, show_config_window, show_scene_window;
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Aries - Chaomeng's soft renderer");

            ImGui::Text("This is some useful text.");
            ImGui::Checkbox("Config Window", &show_config_window);
            ImGui::Checkbox("Scene Manager", &show_scene_window); // 新增场景管理窗口
            ImGui::Checkbox("Model Loader", &show_loader_window);

            ImGui::Text("windows width: %d, height: %d", config.width, config.height);
            ImGui::Text("framebuffer width: %d, height: %d", config.framebuffer_width, config.framebuffer_height);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&config.clear_color);

            if (ImGui::Button("Button"))
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Checkbox("显示坐标系", &pipeline->showCoordinateSystem);

            if (scene->directionalShadow && ImGui::Button("[DEBUG] 保存深度图")) {
                scene->directionalShadow->SaveShadowMap("shadow_map.png");
                std::cout << "[DEBUG] 深度图已保存为 shadow_map.png" << std::endl;
            }

            ImGui::Text("三角形数量: %lld", pipeline->triangleCount);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.f / config.io.Framerate, config.io.Framerate);
            ImGui::Text("Pipeline current render FPS %.3f ms/frame (%.1f FPS)", 1000.f * pipeline->frameTime, 1.f / pipeline->frameTime);

            ImGui::End();
        }

        // 场景管理器窗口
        if (show_scene_window) {
            ShowSceneManagerWindow(&show_scene_window);
        }

        // 模型加载器窗口
        if (show_loader_window) {
            ShowModelLoaderWindow(&show_loader_window);
        }

        // 相机控制
        auto camera = scene->GetCamera();
        if (!config.io.WantCaptureKeyboard) {
            const float cameraSpeed = 2.0f * deltaTime;
            if (glfwGetKey(config.window, GLFW_KEY_W) == GLFW_PRESS)
                camera->Position += Vector4f(camera->Up.x(), camera->Up.y(), camera->Up.z(), 0.f) * cameraSpeed;
            if (glfwGetKey(config.window, GLFW_KEY_S) == GLFW_PRESS)
                camera->Position -= Vector4f(camera->Up.x(), camera->Up.y(), camera->Up.z(), 0.f) * cameraSpeed;
            const Vector3f right = camera->Direction.cross(Vector3f(0.f, 1.f, 0.f)).normalized();
            if (glfwGetKey(config.window, GLFW_KEY_A) == GLFW_PRESS)
                camera->Position -= Vector4f(right.x(), right.y(), right.z(), 0.f) * cameraSpeed;
            if (glfwGetKey(config.window, GLFW_KEY_D) == GLFW_PRESS)
                camera->Position += Vector4f(right.x(), right.y(), right.z(), 0.f) * cameraSpeed;
        }

        // 配置窗口
        if (show_config_window) {
            ShowConfigWindow(&show_config_window, camera);
        }

        pipeline->Render(scene, scene->GetCamera());
        pipeline->Draw();
    }

    void Application::LoadModel(const std::string& filename) {
        std::cout << "[Application] 加载obj模型文件：" << filename << std::endl;
        try {
            auto model = ObjLoader::LoadModel(filename);
            scene->AddModel(model);
        } catch (const std::exception& e) {
            std::cerr << "[Application] 加载obj模型失败: " << e.what() << std::endl;
        }
    }

    void Application::ProcessMouseButton(int button, int action) {
        auto config = SharedConfigManager::GetConfig();
        
        // 如果ImGui正在使用鼠标，则不处理鼠标输入
        if (config->io.WantCaptureMouse)
            return;

        if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            if (action == GLFW_PRESS) {
                mouseMiddlePressed = true;
                
                // 获取当前鼠标位置作为起始点
                double xpos, ypos;
                glfwGetCursorPos(config->window, &xpos, &ypos);
                lastMouseX = static_cast<float>(xpos);
                lastMouseY = static_cast<float>(ypos);
            }
            else if (action == GLFW_RELEASE) {
                mouseMiddlePressed = false;
            }
        }
    }

    void Application::ProcessMouseInput(double xpos, double ypos) {
        auto config = SharedConfigManager::GetConfig();
        
        // 如果ImGui正在使用鼠标，则不处理鼠标输入
        if (config->io.WantCaptureMouse)
            return;
            
        if (mouseMiddlePressed) {
            // 计算鼠标移动的偏移量
            auto xoffset = static_cast<float>(xpos - lastMouseX);
            auto yoffset = static_cast<float>(lastMouseY - ypos); // 反转y轴，因为y坐标是从上到下的
            
            lastMouseX = static_cast<float>(xpos);
            lastMouseY = static_cast<float>(ypos);
            
            // 应用鼠标灵敏度
            xoffset *= mouseSensitivity;
            yoffset *= mouseSensitivity;
            
            // 获取相机
            auto camera = scene->GetCamera();
            
            // 更新相机欧拉角
            float yaw = camera->eluaAngle.x(); // 初始值与界面保持一致
            float pitch = camera->eluaAngle.y();
            
            // 累加偏移量
            yaw -= xoffset;
            pitch += yoffset;

            // 保持Yaw在[-180, 180]范围内
            while (yaw > 180.0f)
                yaw -= 360.0f;

            while (yaw < -180.0f)
                yaw += 360.0f;
            
            // 限制俯仰角防止翻转
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
            
            camera->eluaAngle.x() = yaw;   // 更新相机的Yaw
            camera->eluaAngle.y() = pitch; // 更新相机的Pitch
        }
    }

    void Application::ProcessMouseScroll(double yoffset) {
        auto config = SharedConfigManager::GetConfig();
        
        // 如果ImGui正在使用鼠标，则不处理鼠标输入
        if (config->io.WantCaptureMouse)
            return;
        
        // 滚轮滚动时，相机前后移动
        auto camera = scene->GetCamera();
        const float scrollSpeed = 0.5f; // 滚轮灵敏度
        camera->Position += Vector4f(camera->Direction.x(), camera->Direction.y(), camera->Direction.z(), 0.f) * yoffset * scrollSpeed;
    }

    // 场景管理器窗口
    void Application::ShowSceneManagerWindow(bool* p_open) {
        ImGui::Begin("Scene Manager", p_open, ImGuiWindowFlags_AlwaysAutoResize);

        if (scene->models.empty()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "场景中暂无模型");
            ImGui::Text("请使用 Model Loader 加载模型");
            ImGui::End();
            return;
        }

        ImGui::Text("场景中的模型:");
        ImGui::Separator();

        static std::string selectedModel = "";
        static std::string selectedShape = "";
        static std::string modelToDelete = ""; // 用于标记要删除的模型

        // 遍历所有模型
        for (auto& [modelName, model] : scene->models) {
            // 模型节点（可折叠）
            bool modelNodeOpen = ImGui::TreeNode(("Model: " + modelName).c_str());
            
            // 右键菜单
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("删除模型", nullptr, false, true)) {
                    modelToDelete = modelName; // 标记要删除的模型
                }
                if (ImGui::MenuItem("重置变换")) {
                    // 重置模型变换到默认状态
                    model->position = Vector3f(0.0f, 0.0f, 0.0f);
                    model->rotation = Vector3f(0.0f, 0.0f, 0.0f);
                    model->scale = Vector3f(1.0f, 1.0f, 1.0f);
                }
                if (ImGui::MenuItem("复制模型")) {
                    // 复制模型
                    std::string newName = modelName + "_copy";
                    int copyIndex = 1;
                    while (scene->models.find(newName) != scene->models.end()) {
                        newName = modelName + "_copy_" + std::to_string(copyIndex++);
                    }
                    scene->CopyModel(modelName, newName);
                }
                ImGui::EndPopup();
            }

            if (modelNodeOpen) {
                ImGui::PushID(modelName.c_str());

                // 模型信息显示
                ImGui::Text("Shapes: %zu", model->shapes.size());
                ImGui::Text("Total Triangles: %zu", [&model]() {
                    size_t totalTriangles = 0;
                    for (const auto& shape : model->shapes) {
                        totalTriangles += shape->mesh.size();
                    }
                    return totalTriangles;
                }());

                ImGui::Separator();

                // 模型变换控制
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::DragFloat3("Position", model->position.data(), 0.01f);
                    ImGui::DragFloat3("Rotation", model->rotation.data(), 0.2f, -180.0f, 180.0f);
                    ImGui::DragFloat3("Scale", model->scale.data(), 0.01f, 0.1f, 10.0f);

                    // 快速操作按钮
                    if (ImGui::Button("Reset Position")) {
                        model->position = Vector3f(0.0f, 0.0f, 0.0f);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset Rotation")) {
                        model->rotation = Vector3f(0.0f, 0.0f, 0.0f);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset Scale")) {
                        model->scale = Vector3f(1.0f, 1.0f, 1.0f);
                    }
                }

                ImGui::Separator();

                // 显示模型的 Shapes
                if (ImGui::CollapsingHeader("Shapes", ImGuiTreeNodeFlags_DefaultOpen)) {
                    for (size_t i = 0; i < model->shapes.size(); ++i) {
                        auto& shape = model->shapes[i];
                        std::string shapeId = modelName + "_shape_" + std::to_string(i);
                        
                        // Shape 节点
                        bool shapeNodeOpen = ImGui::TreeNode(("Shape: " + shape->name).c_str());
                        
                        // Shape 右键菜单
                        if (ImGui::BeginPopupContextItem()) {
                            if (ImGui::MenuItem("隐藏 Shape")) {
                                // 假设 Shape 有 visible 属性
                                // shape->visible = false;
                            }
                            if (ImGui::MenuItem("重置材质")) {
                                ApplyDefaultMaterial(std::dynamic_pointer_cast<ShadowedBlinnPhongMaterial>(shape->material)->property);
                            }
                            ImGui::EndPopup();
                        }
                        
                        if (shapeNodeOpen) {
                            ImGui::PushID(shapeId.c_str());

                            // 显示 Shape 信息
                            ImGui::Text("Triangles: %zu", shape->mesh.size());
                            
                            // 材质编辑区域
                            if (ImGui::CollapsingHeader("Material Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                                ShowMaterialEditor(shape);
                            }

                            ImGui::PopID();
                            ImGui::TreePop();
                        }
                    }
                }

                ImGui::PopID();
                ImGui::TreePop();
            }
            ImGui::Separator();
        }

        // 延迟删除模型（避免在迭代时修改容器）
        if (!modelToDelete.empty()) {
            // 显示确认对话框
            ImGui::OpenPopup("Confirm Delete");
        }

        // 删除确认对话框
        if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("确定要删除模型 '%s' 吗？", modelToDelete.c_str());
            ImGui::Text("此操作无法撤销！");
            ImGui::Separator();

            if (ImGui::Button("确定删除", ImVec2(120, 0))) {
                // ✅ 使用 Scene::RemoveModel 方法
                scene->RemoveModel(modelToDelete);
                std::cout << "[Scene] 已删除模型: " << modelToDelete << std::endl;
                
                modelToDelete.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("取消", ImVec2(120, 0))) {
                modelToDelete.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // 批量操作区域
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Batch Operations")) {
            if (ImGui::Button("删除所有模型")) {
                ImGui::OpenPopup("Confirm Delete All");
            }
            
            if (ImGui::BeginPopupModal("Confirm Delete All", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("确定要删除所有模型吗？");
                ImGui::Text("此操作无法撤销！");
                ImGui::Separator();

                if (ImGui::Button("确定删除", ImVec2(120, 0))) {
                    scene->ClearObjects(); // 使用现有的清除方法
                    std::cout << "[Scene] 已清除所有模型" << std::endl;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("取消", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("应用默认材质到所有")) {
                for (auto& [modelName, model] : scene->models) {
                    for (auto& shape : model->shapes) {
                        auto material = std::dynamic_pointer_cast<ShadowedBlinnPhongMaterial>(shape->material);
                        if (material) {
                            ApplyDefaultMaterial(material->property);
                        }
                    }
                }
            }
        }

        if (ImGui::Button("关闭窗口")) 
            *p_open = false;

        ImGui::End();
    }

    // 材质编辑器
    void Application::ShowMaterialEditor(sptr<Shape> shape) {
        auto material = std::dynamic_pointer_cast<ShadowedBlinnPhongMaterial>(shape->material);
        if (!material) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "材质类型不匹配！");
            return;
        }

        auto& prop = material->property;

        // 基础材质属性
        if (ImGui::CollapsingHeader("Basic Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Shininess", &prop.shininess, 0.0f, 128.0f);
            
            ImGui::ColorEdit3("Ambient", prop.ambient.data());
            ImGui::SliderFloat("Ambient Intensity", &prop.ambientIntensity, 0.0f, 1.0f);
            
            ImGui::ColorEdit3("Diffuse", prop.diffuse.data());
            ImGui::SliderFloat("Diffuse Intensity", &prop.diffuseIntensity, 0.0f, 1.0f);
            
            ImGui::ColorEdit3("Specular", prop.specular.data());
            ImGui::SliderFloat("Specular Intensity", &prop.specularIntensity, 0.0f, 1.0f);
        }

        // 阴影属性
        if (ImGui::CollapsingHeader("Shadow Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderFloat("Shadow Intensity", &prop.shadowIntensity, 0.0f, 1.0f);
            ImGui::InputFloat("Shadow Bias", &prop.shadowBias, 0.0001f, 0.001f, "%.6f");
            
            ImGui::SliderInt("PCF Samples", &prop.pcfSamples, 1, 5);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("1 = 硬阴影\n3 = 软阴影 (3x3)\n5 = 高质量软阴影 (5x5)");
            }
            
            ImGui::SliderFloat("Distance Attenuation", &prop.shadowDistanceAttenuation, 0.0f, 50.0f);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("控制环境光阴影的距离衰减速度");
            }
            
            ImGui::SliderFloat("Min Shadow Intensity", &prop.shadowMinIntensity, 0.0f, 1.0f);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("远距离阴影的最小强度");
            }
            
            /*ImGui::SliderFloat("Max Shadow Distance", &prop.shadowMaxDistance, 0.01f, 0.5f);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("阴影距离衰减的最大范围");
            }*/
        }

        // 材质预设
        if (ImGui::CollapsingHeader("Material Presets")) {
            if (ImGui::Button("金属材质")) {
                ApplyMetallicMaterial(prop);
            }
            ImGui::SameLine();
            if (ImGui::Button("塑料材质")) {
                ApplyPlasticMaterial(prop);
            }
            ImGui::SameLine();
            if (ImGui::Button("橡胶材质")) {
                ApplyRubberMaterial(prop);
            }
            
            if (ImGui::Button("玉石材质")) {
                ApplyJadeMaterial(prop);
            }
            ImGui::SameLine();
            if (ImGui::Button("默认材质")) {
                ApplyDefaultMaterial(prop);
            }
        }

        // 纹理信息
        if (ImGui::CollapsingHeader("Texture Info")) {
            if (prop.texture) {
                ImGui::Text("Texture: %dx%d", prop.texture->GetWidth(), prop.texture->GetHeight());
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No texture");
            }
        }
    }

    // 简化的配置窗口（主要是相机和光源）
    void Application::ShowConfigWindow(bool* p_open, sptr<Camera> camera) {
        ImGui::Begin("Camera & Light Config", p_open);

        const float PI = std::numbers::pi_v<float>;

        // 相机设置
        if (ImGui::CollapsingHeader("Camera Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Camera Position", camera->Position.data(), 0.1f);
            ImGui::SliderFloat("Yaw   (°)", &camera->eluaAngle.x(), -180.0f, 180.0f);
            ImGui::SliderFloat("Pitch (°)", &camera->eluaAngle.y(), -89.0f, 89.0f);
            ImGui::SliderFloat("Roll  (°)", &camera->eluaAngle.z(), -180.0f, 180.0f);
            ImGui::SliderFloat("FOV (°)", &camera->Fov, 1.0f, 180.0f);
            ImGui::InputFloat("Near Plane", &camera->Near, 0.0001f, 0.001f, "%.6f");
            ImGui::InputFloat("Far Plane", &camera->Far, 0.01f, 1.f, "%.6f");
        }

        // 光源设置
        if (ImGui::CollapsingHeader("Light Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto light = scene->mainLight;
            if (light) {
                static float lightYaw = -90.0f;
                static float lightPitch = 0.0f;
                static float lightRoll = -180.f;
                
                ImGui::SliderFloat("Light Yaw   (°)", &lightYaw, -180.0f, 180.0f);
                ImGui::SliderFloat("Light Pitch (°)", &lightPitch, -89.0f, 89.0f);
                ImGui::SliderFloat("Light Roll  (°)", &lightRoll, -180.0f, 180.0f);
                
                // 更新光源方向
                float lightYawRad = lightYaw * PI / 180.0f;
                float lightPitchRad = lightPitch * PI / 180.0f;
                Vector3f lightFront;
                lightFront.x() = cosf(lightPitchRad) * cosf(lightYawRad);
                lightFront.y() = sinf(lightPitchRad);
                lightFront.z() = cosf(lightPitchRad) * sinf(lightYawRad);
                light->direction = lightFront.normalized();
                scene->directionalShadow->SetLightDirection(light->direction);
            } else {
                ImGui::Text("No light source");
            }
        }

        if (ImGui::Button("Close")) 
            *p_open = false;

        ImGui::End();
    }

    // 分离的模型加载器窗口
    void Application::ShowModelLoaderWindow(bool* p_open) {
        ImGui::Begin("Model Loader", p_open, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Load 3D Model Files");
        ImGui::Separator();
        
        // 文件路径输入框
        static char filePath[512] = R"(F:\绝区零安比\无标题.obj)";
        ImGui::InputTextWithHint("##filepath", "Enter file path or browse...", filePath, sizeof(filePath));

        ImGui::SameLine();
        if (ImGui::Button("Browse...")) {
            IGFD::FileDialogConfig config;
            config.path = "F:\\";
            config.countSelectionMax = 1;
            config.flags = ImGuiFileDialogFlags_Modal;
            
            ImGuiFileDialog::Instance()->OpenDialog(
                "ChooseModelFile",
                "Choose 3D Model File",
                ".obj,.fbx,.dae,.ply,.stl",
                config
            );
        }
        
        // 显示文件对话框
        if (ImGuiFileDialog::Instance()->Display("ChooseModelFile", 
            ImGuiWindowFlags_None, 
            ImVec2(800, 600))) {
            
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string selectedFile = ImGuiFileDialog::Instance()->GetFilePathName();
                strncpy_s(filePath, selectedFile.c_str(), sizeof(filePath) - 1);
                filePath[sizeof(filePath) - 1] = '\0';
            }
            ImGuiFileDialog::Instance()->Close();
        }
        
        ImGui::Separator();
        
        if (ImGui::Button("Load Model")) {
            if (strlen(filePath) > 0) {
                LoadModel(filePath);
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear All Models")) {
            ImGui::OpenPopup("Confirm Clear All");
        }
        
        // 清除所有模型的确认对话框
        if (ImGui::BeginPopupModal("Confirm Clear All", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("确定要清除所有已加载的模型吗？");
            ImGui::Text("此操作无法撤销！");
            ImGui::Separator();

            if (ImGui::Button("确定清除", ImVec2(120, 0))) {
                scene->ClearObjects();
                std::cout << "[Scene] 已清除所有模型" << std::endl;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("取消", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        ImGui::Separator();
        
        // 显示已加载的模型信息
        if (!scene->models.empty()) {
            ImGui::Text("Loaded Models:");
            for (const auto& [name, model] : scene->models) {
                ImGui::BulletText("%s (%zu shapes)", name.c_str(), model->shapes.size());
            }
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No models loaded");
        }

        if (ImGui::Button("Close")) 
            *p_open = false;
        
        ImGui::End();
    }

    // 材质预设函数
    void Application::ApplyDefaultMaterial(ShadowedBlinnPhongMaterial::property_t& prop) {
        prop.shininess = 64.0f;
        prop.ambient = Vector3f(1.f, 1.f, 1.f);
        prop.diffuse = Vector3f(1.f, 1.f, 1.f);
        prop.specular = Vector3f(1.f, 1.f, 1.f);
        prop.ambientIntensity = 0.43f;
        prop.diffuseIntensity = 0.47f;
        prop.specularIntensity = 0.1f;
    }

    void Application::ApplyMetallicMaterial(ShadowedBlinnPhongMaterial::property_t& prop) {
        prop.shininess = 128.0f;
        prop.ambient = Vector3f(0.1f, 0.1f, 0.1f);
        prop.diffuse = Vector3f(0.4f, 0.4f, 0.4f);
        prop.specular = Vector3f(1.0f, 1.0f, 1.0f);
        prop.ambientIntensity = 0.2f;
        prop.diffuseIntensity = 0.3f;
        prop.specularIntensity = 0.8f;
    }

    void Application::ApplyPlasticMaterial(ShadowedBlinnPhongMaterial::property_t& prop) {
        prop.shininess = 32.0f;
        prop.ambient = Vector3f(0.3f, 0.3f, 0.3f);
        prop.diffuse = Vector3f(0.7f, 0.7f, 0.7f);
        prop.specular = Vector3f(0.5f, 0.5f, 0.5f);
        prop.ambientIntensity = 0.4f;
        prop.diffuseIntensity = 0.7f;
        prop.specularIntensity = 0.4f;
    }

    void Application::ApplyRubberMaterial(ShadowedBlinnPhongMaterial::property_t& prop) {
        prop.shininess = 8.0f;
        prop.ambient = Vector3f(0.4f, 0.4f, 0.4f);
        prop.diffuse = Vector3f(0.8f, 0.8f, 0.8f);
        prop.specular = Vector3f(0.1f, 0.1f, 0.1f);
        prop.ambientIntensity = 0.5f;
        prop.diffuseIntensity = 0.8f;
        prop.specularIntensity = 0.1f;
    }

    void Application::ApplyJadeMaterial(ShadowedBlinnPhongMaterial::property_t& prop) {
        prop.shininess = 96.0f;
        prop.ambient = Vector3f(0.135f, 0.2225f, 0.1575f);
        prop.diffuse = Vector3f(0.54f, 0.89f, 0.63f);
        prop.specular = Vector3f(0.316228f, 0.316228f, 0.316228f);
        prop.ambientIntensity = 0.6f;
        prop.diffuseIntensity = 0.7f;
        prop.specularIntensity = 0.5f;
    }
}