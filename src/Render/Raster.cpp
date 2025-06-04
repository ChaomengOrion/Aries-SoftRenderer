/// FileName: Raster.cpp
/// Date: 2025/05/11
/// Author: ChaomengOrion

#include <iostream>
#include "Raster.hpp"
#include <cstring>

// 初始化像素缓冲区
void Raster::InitBuffers(int width, int height) {
    std::cout << "[Raster]" << "帧缓冲区被设置为" << width << 'x' << height << '\n';
    swW = width;
    swH = height;
    swBuffer.assign(swW * swH * 4, 0x00); // RGBA
    //swBuffer[1].assign(swW * swH * 4, 0x00); // RGBA
    //swBuffer[2].assign(swW * swH * 4, 0x00); // RGBA
    glGenTextures(1, &swTex);
    glBindTexture(GL_TEXTURE_2D, swTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, swW, swH, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    //打开混合，以便支持透明度
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Raster::ClearCurrentBuffer() {
    std::memset(swBuffer.data(), 0x00, swW * swH * 4);
}

// CPU 端写像素
void Raster::SetPixel(int x, int y, color_t r, color_t g, color_t b, color_t a) {
    if (x < 0 || x >= swW || y < 0 || y >= swH) return;
    int idx = (y * swW + x) * 4;
    swBuffer[idx + 0] = r;
    swBuffer[idx + 1] = g;
    swBuffer[idx + 2] = b;
    swBuffer[idx + 3] = a;
}

// 切换缓冲区
/*void Raster::SwapBuffers() {
    swLast = swCur; // 保存当前缓冲区索引
    swCur = (swCur + 1) % 3; // 切换到下一个缓冲区
}*/

// 上传像素缓冲区到 GPU
void Raster::UploadToGPU() {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, swW, swH, GL_RGBA, GL_UNSIGNED_BYTE, swBuffer.data());
}

// 绘制全屏四边形，采样像素缓冲区
void Raster::Draw() {
    glEnable(GL_TEXTURE_2D);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(-1, -1);
    glTexCoord2f(1, 0);
    glVertex2f(1, -1);
    glTexCoord2f(1, 1);
    glVertex2f(1, 1);
    glTexCoord2f(0, 1);
    glVertex2f(-1, 1);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_TEXTURE_2D);
}