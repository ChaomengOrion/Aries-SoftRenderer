/// FileName: Texture.hpp
/// Date: 2025/06/01
/// Author: ChaomengOrion

#pragma once
#include <string>
#include <vector>
#include <Core>

using Eigen::Vector3f;

class Texture {
public:
    // 从文件加载 PNG（或其他 stbi 支持格式）
    // 返回 true 表示加载成功
    bool LoadFromFile(const std::string& filename);

    // 根据 UV 坐标获取颜色，u,v 在 [0,1] 区间循环
    // 返回 Vector3f(r,g,b)，范围 [0,1]
    Vector3f Sample(float u, float v) const;

private:
    int width = 0, height = 0, channels = 0;
    std::vector<uint8_t> data;  // 原始像素数据（行主序）
};