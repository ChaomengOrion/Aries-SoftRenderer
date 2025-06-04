/// FileName: Application.cpp
/// Date: 2025/05/11
/// Author: ChaomengOrion

#include "Application.hpp"

#include "SharedConfig.hpp"
#include "ObjLoader.hpp"

namespace aries {
    void Application::Init() {
        pipeline = std::make_shared<Pipeline>();
        scene = std::make_shared<Scene>("Test Scene", pipeline);

        pipeline->InitFrameSize();
        scene->SetTestCamera(); // 设置测试相机
        scene->SetTestLight(); // 设置测试光源

        
        if (!renderThreadRunning) {
            StartRenderThread(); // 启动渲染线程
        }
    }

    void Application::StartRenderThread() {
        renderThreadRunning = true;
        renderThread = std::thread([this]() {
            while (renderThreadRunning) {
                pipeline->Draw(scene, scene->GetCamera());
            }
        });
    }

    void Application::OnUpdate() {
        auto config = SharedConfigManager::GetConfig();

        static bool show_another_window, show_config_window;
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Aries - Chaomeng's soft renderer"); // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text."); // Display some text (you can use a format
                                                    // strings too)
            ImGui::Checkbox("Config Window",
                            &show_config_window); // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::Text("windows width: %d, height: %d", config->width, config->height);
            ImGui::Text("framebuffer width: %d, height: %d", config->framebuffer_width, config->framebuffer_height);

            ImGui::SliderFloat(
                "float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color",
                            (float*)&config->clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return
                                        // true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("三角形数量: %lld", pipeline->triangleCount);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.f / config->io.Framerate, config->io.Framerate);
            ImGui::Text("Pipeline Render FPS %.3f ms/frame (%.1f FPS)", 1000.f * pipeline->frameTime, 1.f / pipeline->frameTime);
            ImGui::Checkbox("启用渲染线程", &renderThreadRunning);
            if (renderThreadRunning && !renderThread.joinable()) {
                StartRenderThread(); // 确保渲染线程在运行
            } else if (!renderThreadRunning && renderThread.joinable()) {
                renderThread.join(); // 等待渲染线程结束
            }
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window) {
            ImGui::Begin(
                "Another Window",
                &show_another_window); // Pass a pointer to our bool variable (the window will have
                                    // a closing button that will clear the bool when clicked)

            // 显示模型加载
            ImGui::Text("加载模型");
            static char filePath[128] = "C:\\Users\\21988\\Downloads\\绝区零安比_爱给网_aigei_com\\无标题.obj";
            ImGui::InputText("file path", filePath, 128);
            if (ImGui::Button("Load Model")) {
                LoadModel(filePath); // 加载模型
            }

            if (ImGui::Button("Clear Objects")) {
                scene->ClearObjects(); // 清除当前场景中的所有对象
            }

            if (ImGui::Button("Close Me")) show_another_window = false;
            ImGui::End();
        }

        auto camera = scene->GetCamera();
        
        // 3. Show another simple window.
        if (show_config_window) {
            ImGui::Begin("Config Window", &show_config_window);
            static float cameraPos[3] = {0, 0, 10};

            const float PI = std::numbers::pi_v<float>;

            ImGui::DragFloat3("Camera Position", cameraPos, 0.1f);
            ImGui::SliderFloat("Yaw   (°)",   &camera->eluaAngle.x(),   -180.0f, 180.0f);
            ImGui::SliderFloat("Pitch (°)",   &camera->eluaAngle.y(), -89.0f,   89.0f);
            ImGui::SliderFloat("Roll  (°)",   &camera->eluaAngle.z(),  -180.0f, 180.0f);
            ImGui::SliderFloat("FOV (°)", &camera->Fov, 1.0f, 180.0f);
            ImGui::InputFloat("近平面裁剪", &camera->Near);
            ImGui::InputFloat("远平面裁剪", &camera->Far);

            // 更新相机位置
            camera->Position = Vector4f(cameraPos[0], cameraPos[1], cameraPos[2], 1.0f);

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
                static float shininess = 8.f; // 默认高光系数
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
                            auto mat = std::static_pointer_cast<BlinnPhongMaterial>(shape->material);
                            mat->property.shininess = shininess;
                            mat->property.ambientIntensity = ambientIntensity;
                            mat->property.diffuseIntensity = diffuseIntensity;
                            mat->property.specularIntensity = specularIntensity;
                            mat->property.ambient = Vector3f(ambientColor[0], ambientColor[1], ambientColor[2]);
                            mat->property.diffuse = Vector3f(diffuseColor[0], diffuseColor[1], diffuseColor[2]);
                            mat->property.specular = Vector3f(specularColor[0], specularColor[1], specularColor[2]);
                        }
                    }
                }
            }

            if (ImGui::Button("Close Me")) show_config_window = false;
            ImGui::End();
        }
        pipeline->OnUpdate();
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
            yaw += xoffset;
            pitch += yoffset;
            
            // 限制俯仰角防止翻转
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
            
            camera->eluaAngle.x() = yaw;   // 更新相机的Yaw
            camera->eluaAngle.y() = pitch; // 更新相机的Pitch
        }
    }
}