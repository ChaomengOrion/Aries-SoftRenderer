/// FileName: main.cpp
/// Date: 2025/05/11
/// Author: ChaomengOrion
/// Description: 修改自 Dear ImGui 的 GLFW + OpenGL3 示例，作为程序入口

// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs,
// OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "Application.hpp"

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of
// testing and compatibility with old VS compilers. To link with VS2010-era libraries, VS2015+
// requires linking with legacy_stdio_definitions.lib, which we do using this pragma. Your own
// project should not be affected, as you are likely to link with a newer binary of GLFW that is
// adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#    pragma comment(lib, "legacy_stdio_definitions")
#endif

// This example can also compile and run with Emscripten! See 'Makefile.emscripten' for details.
#ifdef __EMSCRIPTEN__
#    include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

aries::Application* g_app = nullptr;

// 鼠标移动回调
static void mouse_cursor_callback(GLFWwindow* window, double xpos, double ypos) {
    // 先让ImGui处理事件
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

    if (g_app) {
        g_app->ProcessMouseInput(xpos, ypos);
    }
}

// 鼠标按钮回调
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    // 先让ImGui处理事件
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    
    if (g_app) {
        g_app->ProcessMouseButton(button, action);
    }
}

// 鼠标滚轮回调
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    if (g_app)
        g_app->ProcessMouseScroll(yoffset);
}

// Main code
int main(int, char**) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char* glsl_version = "#version 300 es";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    // Create window with graphics context
    GLFWwindow* window =
        glfwCreateWindow(1920, 1027, "Aries - Chaomeng's soft renderer (Based on Imgui+GLFW) [v2025-6.3+1]", nullptr, nullptr);
    if (window == nullptr) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

#pragma region 样式设置
    ImGui::StyleColorsDark(); // 暗色主题
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplGlfw_InstallEmscriptenCallbacks(window, "#canvas");
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    // 获取窗口缩放比例（按主屏幕dpi）
    float xscale, yscale;
    glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &xscale, &yscale);
    // 字体采用微软雅黑
    ImFont* font = io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\msyh.ttc)",
                                 18.0f * xscale,
                                 nullptr,
                                 io.Fonts->GetGlyphRangesChineseFull());
    IM_ASSERT(font != nullptr);
#pragma endregion

#pragma region 主循环体
    // Main loop

    std::shared_ptr<SharedConfig> config = std::make_shared<SharedConfig>(window, io);
    config->clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f); // 背景剔除色
    glfwGetFramebufferSize(window, &config->width, &config->height);
    config->framebuffer_width = config->width;
    config->framebuffer_height = config->height;
    SharedConfigManager::BindConfig(config);

    aries::Application app;
    g_app = &app; // 设置全局应用实例
    app.Init();

    // 注册鼠标回调，覆盖默认的鼠标回调
    glfwSetCursorPosCallback(window, mouse_cursor_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a
    // fopen() of the imgui.ini file. You may manually call LoadIniSettingsFromMemory() to load
    // settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        // 拉取事件
        glfwPollEvents();
        
        // 处理窗口最小化
        if (glfwGetWindowAttrib(config->window, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // 更新窗口大小
        glfwGetFramebufferSize(window, &config->width, &config->height);
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Rendering
        glViewport(0, 0, config->width, config->height);
        glClearColor(config->clear_color.x * config->clear_color.w,
            config->clear_color.y * config->clear_color.w,
            config->clear_color.z * config->clear_color.w,
            config->clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);

        // 执行更新
        app.OnUpdate(*config);
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(config->window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif
#pragma endregion

#pragma region 资源释放
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
#pragma endregion

    return 0;
}
