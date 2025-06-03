/// FileName: SharedConfig.hpp
/// Date: 2025/05/11
/// Author: ChaomengOrion

#pragma once

#include <memory>
#include "ImguiCommon.hpp"

struct SharedConfig {
    SharedConfig(GLFWwindow* window, ImGuiIO& io) : window(window), io(io) {}
    GLFWwindow* window;
    ImGuiIO& io;
    int width, height;
    int framebuffer_width, framebuffer_height;
    ImVec4 clear_color;
};

class SharedConfigManager {
private:
    static std::shared_ptr<SharedConfig> s_instance;
public:
    static void BindConfig(std::shared_ptr<SharedConfig> config);
    static std::shared_ptr<const SharedConfig> GetConfig(); 
};