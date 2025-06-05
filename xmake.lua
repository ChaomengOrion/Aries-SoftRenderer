-- 设置项目信息
set_project("SoftRender")
set_version("1.0.0")

-- 设置最低xmake版本
set_xmakever("2.7.0")

-- 强制指定g++编译器
set_toolchains("gcc")
set_toolset("cc", "gcc")
set_toolset("cxx", "g++")
set_toolset("ld", "g++")

-- 设置默认项目选项
set_languages("c++23")

-- 添加构建模式
add_rules("mode.debug", "mode.release")

-- 默认构建模式
set_defaultmode("release")

if is_mode("debug") then
    add_cxxflags("-Og")
    -- add_cxxflags("-Wall", "-Wextra")  -- 开启所有警告
    add_defines("DEBUG")
    set_symbols("debug")  -- 开启调试符号
elseif is_mode("release") then
    set_optimize("fastest")
    set_symbols("hidden") -- 隐藏符号
end

-- 设置构建目录
set_targetdir("build/$(plat)_$(arch)_$(mode)")

-- 启用OpenMP
add_cxflags("-fopenmp")
if is_plat("windows") then
    add_ldflags("-fopenmp")
else
    add_ldflags("-fopenmp")
end

-- 添加包含目录
add_includedirs("libs")
add_includedirs("libs/imgui")
add_includedirs("libs/glfw-3.4.bin.WIN64/include")
add_includedirs("libs/eigen")
add_includedirs("libs/boost")
add_includedirs("libs/tiny_obj_loader")
add_includedirs("libs/stb")
add_includedirs("libs/ImGuiFileDialog")

-- 定义目标
target("Aries")
    set_kind("binary")
    
    -- 添加源文件
    add_files("src/*.cpp")
    add_files("src/Render/*.cpp")
    add_files("libs/imgui/*.cpp")
    add_files("libs/ImGuiFileDialog/*.cpp")
    
    -- 添加库路径和链接库
    if is_plat("windows") then
        add_linkdirs("libs/glfw-3.4.bin.WIN64/lib-mingw-w64")
        add_links("glfw3", "opengl32", "gdi32")
    else
        add_links("glfw3", "GL", "X11", "pthread", "dl")
    end
    
    -- 平台特定配置
    on_load(function (target)
        -- Windows平台特定配置
        if is_plat("windows") then
            -- 如果在MinGW环境中设置正确的链接器配置
            if is_subhost("mingw") then
                target:add("ldflags", "-static-libgcc", "-static-libstdc++", {force = true})
            end
        end
    end)