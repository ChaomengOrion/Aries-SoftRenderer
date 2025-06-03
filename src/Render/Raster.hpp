/// FileName: Raster.hpp
/// Date: 2025/05/11
/// Author: ChaomengOrion
#pragma once

#include "GL/gl.h"
#include <vector>
#include <mutex>

class Raster {
public:
    using color_t = unsigned char; // 0-255 单通道灰度值

private:
    int swW = 0, swH = 0;
    std::vector<color_t> swBuffer[3]; // 三重缓冲区
    int swCur = 1, swLast = 0;
    GLuint swTex = 0;
    std::mutex bufferMutex; // 保护双缓冲区的互斥锁

public:
    // 初始化像素缓冲区
    void InitBuffers(int width, int height);

    // 清除像素缓冲区
    void ClearCurrentBuffer();

    // 切换缓冲区
    void SwapBuffers();

    // CPU 端写像素
    void SetPixel(int x, int y, color_t r, color_t g, color_t b, color_t a = 255);

    // 上传像素缓冲区到 GPU
    void UploadToGPU();

    // 绘制全屏四边形，采样像素缓冲区
    void Draw();
};