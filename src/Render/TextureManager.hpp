/// FileName: TextureManager.hpp
/// Date: 2025/05/29
/// Author: ChaomengOrion

#include "Texture.hpp"
#include <memory>
#include <iostream>

class TextureManager {
private:
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;

public:
    TextureManager() = default;

    // 禁止拷贝和赋值
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    // 获取单例实例
    static TextureManager& GetInstance() {
        static TextureManager instance;
        return instance;
    }

    // 加载纹理
    std::shared_ptr<Texture> LoadTexture(const std::string& path) {
        auto it = textures.find(path);
        if (it != textures.end()) {
            return it->second; // 如果已存在，返回现有纹理
        }

        auto texture = std::make_shared<Texture>();
        if (texture->LoadFromFile(path)) {
            textures[path] = texture; // 存储新加载的纹理
            std::cout << "[TextureManager] 纹理加载成功: " << path << std::endl;
            return texture;
        } else {
            throw std::runtime_error("Failed to load texture from " + path);
        }
    }

    // 清除未使用的纹理
    void ClearUnusedTextures() {
        for (auto it = textures.begin(); it != textures.end();) {
            if (it->second.use_count() == 1) { // 如果没有其他引用，删除
                it = textures.erase(it);
            } else {
                ++it;
            }
        }
    }
};