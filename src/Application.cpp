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

        static bool show_loader_window, show_config_window;
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Aries - Chaomeng's soft renderer"); // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text."); // Display some text (you can use a format
                                                    // strings too)
            ImGui::Checkbox("Config Window",
                            &show_config_window); // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_loader_window);

            ImGui::Text("windows width: %d, height: %d", config.width, config.height);
            ImGui::Text("framebuffer width: %d, height: %d", config.framebuffer_width, config.framebuffer_height);

            ImGui::SliderFloat(
                "float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color",
                            (float*)&config.clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return
                                        // true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Checkbox("显示坐标系", &pipeline->showCoordinateSystem);

            ImGui::Text("三角形数量: %lld", pipeline->triangleCount);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.f / config.io.Framerate, config.io.Framerate);
            ImGui::Text("Pipeline current render FPS %.3f ms/frame (%.1f FPS)", 1000.f * pipeline->frameTime, 1.f / pipeline->frameTime);
            /*ImGui::Checkbox("启用渲染线程", &renderThreadRunning);
            if (renderThreadRunning && !renderThread.joinable()) {
                StartRenderThread(); // 确保渲染线程在运行
            } else if (!renderThreadRunning && renderThread.joinable()) {
                renderThread.join(); // 等待渲染线程结束
            }*/
            ImGui::End();
        }

        //* 显示模型加载器窗口
        if (show_loader_window) {
            ImGui::Begin("模型加载器", &show_loader_window, ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::Text("加载 3D 模型文件");
            ImGui::Separator();
            
            // 文件路径输入框
            static char filePath[512] = R"(F:\绝区零安比\无标题.obj)";
            ImGui::InputTextWithHint("##filepath", "请输入文件路径或点击浏览选择...", filePath, sizeof(filePath));

            ImGui::SameLine();
            if (ImGui::Button("浏览文件...")) {
                // 使用新版本的 FileDialogConfig
                IGFD::FileDialogConfig config;
                config.path = "F:\\";  // 设置默认路径
                config.countSelectionMax = 1;
                config.flags = ImGuiFileDialogFlags_Modal;
                
                // 调用新版本的 OpenDialog
                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChooseModelFile",           // key
                    "选择 3D 模型文件",          // title  
                    ".obj,.fbx,.dae,.ply,.stl",  // filters
                    config                       // config
                );
            }
            
            // 显示文件对话框
            if (ImGuiFileDialog::Instance()->Display("ChooseModelFile", 
                ImGuiWindowFlags_None, 
                ImVec2(800, 600) // 最小尺寸
                )) {
                
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    std::string selectedFile = ImGuiFileDialog::Instance()->GetFilePathName();
                    strncpy_s(filePath, selectedFile.c_str(), sizeof(filePath) - 1);
                    filePath[sizeof(filePath) - 1] = '\0';
                }
                ImGuiFileDialog::Instance()->Close();
            }
            
            ImGui::Separator();
            
            if (ImGui::Button("加载模型")) {
                if (strlen(filePath) > 0) {
                    LoadModel(filePath);
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("清除所有模型")) {
                scene->ClearObjects();
            }
            
            ImGui::Separator();
            
            // 显示已加载的模型信息
            if (!scene->models.empty()) {
                ImGui::Text("已加载的模型:");
                for (const auto& [name, model] : scene->models) {
                    ImGui::BulletText("%s (%zu shapes)", name.c_str(), model->shapes.size());
                }
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "暂无已加载的模型");
            }

            if (ImGui::Button("关闭窗口")) 
                show_loader_window = false;
            
            ImGui::End();
        }

        auto camera = scene->GetCamera();
        
        // 检测键盘输入，移动相机
        if (!config.io.WantCaptureKeyboard) {
            const float cameraSpeed = 2.0f * deltaTime; // 每秒移动2个单位
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

        // 3. Show another simple window.
        if (show_config_window) {
            ImGui::Begin("Config Window", &show_config_window);

            const float PI = std::numbers::pi_v<float>;

            ImGui::DragFloat3("Camera Position", camera->Position.data(), 0.1f);
            ImGui::SliderFloat("Yaw   (°)",   &camera->eluaAngle.x(),   -180.0f, 180.0f);
            ImGui::SliderFloat("Pitch (°)",   &camera->eluaAngle.y(), -89.0f,   89.0f);
            ImGui::SliderFloat("Roll  (°)",   &camera->eluaAngle.z(),  -180.0f, 180.0f);
            ImGui::SliderFloat("FOV (°)", &camera->Fov, 1.0f, 180.0f);
            ImGui::InputFloat("近平面裁剪", &camera->Near);
            ImGui::InputFloat("远平面裁剪", &camera->Far);

            // 光源调整
            auto light = scene->mainLight;
            if (light) {
                ImGui::Text("光源朝向（欧拉角）");
                static float lightYaw   = -90.0f; // 水平角度，度
                static float lightPitch =  0.0f; // 垂直角度，度
                static float lightRoll  = -180.f; // 翻滚角度，度
                ImGui::SliderFloat("Light Yaw   (°)",   &lightYaw,   -180.0f, 180.0f);
                ImGui::SliderFloat("Light Pitch (°)",   &lightPitch, -89.0f,   89.0f);
                ImGui::SliderFloat("Light Roll  (°)",   &lightRoll,  -180.0f, 180.0f);
                // 更新光源方向
                float lightYawRad   = lightYaw   * PI / 180.0f;
                float lightPitchRad = lightPitch * PI / 180.0f;
                Vector3f lightFront;
                lightFront.x() = cosf(lightPitchRad) * cosf(lightYawRad);
                lightFront.y() = sinf(lightPitchRad);
                lightFront.z() = cosf(lightPitchRad) * sinf(lightYawRad);
                light->direction = lightFront.normalized();
            } else {
                ImGui::Text("没有光源");
            }

            if (scene->models.empty()) {
                ImGui::Text("当前场景没有模型");
            } else {
                static float shininess = 64.f; // 默认高光系数
                static float ambientIntensity = 0.43f; // 默认环境光强度
                static float ambientColor[3] = {1.0f, 1.0f, 1.0f}; // 默认环境光颜色
                static float diffuseIntensity = 0.47f; // 默认漫反射强度
                static float diffuseColor[3] = {1.0f, 1.0f, 1.0f}; // 默认漫反射颜色
                static float specularIntensity = 0.1; // 默认镜面反射强度
                static float specularColor[3] = {1.0f, 1.0f, 1.0f}; // 默认镜面反射颜色

                static bool autoApplyMaterial = true; // 自动应用材质
                ImGui::Checkbox("Auto Apply Material", &autoApplyMaterial);
                
                ImGui::SliderFloat("Shininess", &shininess, 0.0f, 128.0f);
                ImGui::SliderFloat("Ambient Intensity", &ambientIntensity, 0.0f, 1.0f);
                ImGui::ColorEdit3("Ambient Color", ambientColor); // 环境光颜色
                ImGui::SliderFloat("Diffuse Intensity", &diffuseIntensity, 0.0f, 1.0f);
                ImGui::ColorEdit3("Diffuse Color", diffuseColor); // 漫反射颜色
                ImGui::SliderFloat("Specular Intensity", &specularIntensity, 0.0f, 1.0f);
                ImGui::ColorEdit3("Specular Color", specularColor); // 镜面反射颜色

                for (const auto& [name, model] : scene->models) {
                    ImGui::Text("Model: %s", name.c_str());
                    
                    // 更新模型的材质
                    for (auto& shape : model->shapes) {
                        if (autoApplyMaterial || (ImGui::Text("name: %s", shape->name.c_str()), ImGui::Button(("Update Material##" + shape->name).c_str()))) {
                            /*auto mat = std::static_pointer_cast<BlinnPhongMaterial>(shape->material);
                            mat->property.shininess = shininess;
                            mat->property.ambientIntensity = ambientIntensity;
                            mat->property.diffuseIntensity = diffuseIntensity;
                            mat->property.specularIntensity = specularIntensity;
                            mat->property.ambient = Vector3f(ambientColor[0], ambientColor[1], ambientColor[2]);
                            mat->property.diffuse = Vector3f(diffuseColor[0], diffuseColor[1], diffuseColor[2]);
                            mat->property.specular = Vector3f(specularColor[0], specularColor[1], specularColor[2]);*/
                        }
                    }
                }
            }

            if (ImGui::Button("Close Me")) show_config_window = false;
            ImGui::End();
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
}