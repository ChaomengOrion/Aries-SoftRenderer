#pragma once

#include "../CommonHeader.hpp"
#include "../Model.hpp"

//! 调试用 
// TODO: 删除
#include "stb_image_write.h"

using namespace aries::model;

namespace aries::shadow {
    
    // 独立的阴影映射渲染器
    class ShadowMapRenderer {
    private:
        int m_width, m_height;
        vector<float> m_depthBuffer;
        Matrix4f m_lightViewMatrix;
        Matrix4f m_lightProjectionMatrix;
        Matrix4f m_viewport;

        // 添加性能优化相关成员
        static constexpr int COARSE_FACTOR = 8; // 粗略深度缓冲的下采样因子
        vector<float> m_coarseDepthBuffer; // 低分辨率深度缓冲用于快速剔除
        int m_coarseWidth, m_coarseHeight;

    public:
        ShadowMapRenderer(int size) : m_width(size), m_height(size) {
            m_depthBuffer.resize(size * size, 1.0f);
            
            // 初始化粗略深度缓冲
            m_coarseWidth = (size + COARSE_FACTOR - 1) / COARSE_FACTOR;
            m_coarseHeight = (size + COARSE_FACTOR - 1) / COARSE_FACTOR;
            m_coarseDepthBuffer.resize(m_coarseWidth * m_coarseHeight, 1.0f);
            
            // 设置视口变换矩阵
            m_viewport << 
                size / 2.0f, 0,           0, size / 2.0f,
                0,           size / 2.0f, 0, size / 2.0f,
                0,           0,           1, 0,
                0,           0,           0, 1;
        }

        // 设置光源的视图和投影矩阵
        void SetLightMatrices(const Matrix4f& view, const Matrix4f& projection) {
            m_lightViewMatrix = view;
            m_lightProjectionMatrix = projection;
        }

        // 清除深度缓冲
        void Clear() {
            std::fill(m_depthBuffer.begin(), m_depthBuffer.end(), 1.0f);
            std::fill(m_coarseDepthBuffer.begin(), m_coarseDepthBuffer.end(), 1.0f);
        }

        //! 保存深度图为图像文件（灰度图）
        void SaveDepthMapAsImage(const std::string& filename) const {
            // 创建 RGB 数据缓冲区
            std::vector<unsigned char> imageData(m_width * m_height * 3);
            
            for (int y = 0; y < m_height; ++y) {
                for (int x = 0; x < m_width; ++x) {
                    int depthIdx = x + y * m_width;
                    int imageIdx = (x + (m_height - y - 1) * m_width) * 3; // 翻转 Y 轴
                    
                    // 将深度值 [-1,1] 映射到 [0,255]
                    float depth = m_depthBuffer[depthIdx];
                    depth = std::clamp((depth + 1.0f) / 2.0f, 0.0f, 1.0f); // 确保在 [0,1] 范围内
                    unsigned char grayValue = static_cast<unsigned char>(depth * 255);
                    
                    // RGB 都设为相同值（灰度图）
                    imageData[imageIdx + 0] = grayValue; // R
                    imageData[imageIdx + 1] = grayValue; // G
                    imageData[imageIdx + 2] = grayValue; // B
                }
            }
            
            // 保存为 PNG 文件
            int result = stbi_write_png(filename.c_str(), m_width, m_height, 3, imageData.data(), m_width * 3);
            
            if (result) {
                std::cout << "[ShadowMapRenderer] 深度图已保存到: " << filename << std::endl;
            } else {
                std::cout << "[ShadowMapRenderer] 错误：无法保存深度图到: " << filename << std::endl;
            }
        }

        // 渲染阴影映射
        void RenderShadowMap(vector<sptr<Shape>>& shapeList) {
            Clear();
            
            Matrix4f lightViewProjection = m_lightProjectionMatrix * m_lightViewMatrix;
            
            // 处理每个形状
            for (auto& shape : shapeList) {
                Matrix4f modelMatrix = shape->model->GetModelMatrix();
                Matrix4f mvp = lightViewProjection * modelMatrix;
                
                // 处理每个三角形
                for (size_t i = 0; i < shape->mesh.size(); i++) {
                    ProcessTriangle(shape->mesh[i], mvp);
                }
            }
        }

        // 采样深度值
        float SampleDepth(float u, float v) const {
            int x = std::clamp((int)(u * m_width), 0, m_width - 1);
            int y = std::clamp((int)(v * m_height), 0, m_height - 1);
            int idx = x + (m_height - y - 1) * m_width;
            return m_depthBuffer[idx];
        }

        // 获取阴影映射纹理
        const vector<float>& GetDepthBuffer() const {
            return m_depthBuffer;
        }

        // 获取光源视图投影矩阵
        Matrix4f GetLightViewProjectionMatrix() const {
            return m_lightProjectionMatrix * m_lightViewMatrix;
        }

        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }

    private:
        // 处理单个三角形（添加更多优化）
        inline void ProcessTriangle(const Triangle& triangle, const Matrix4f& mvp) {
            // 1. 顶点变换到齐次裁剪空间
            Vector4f v0 = mvp * Vector4f(triangle.vertex[0].x(), triangle.vertex[0].y(), triangle.vertex[0].z(), 1.0f);
            Vector4f v1 = mvp * Vector4f(triangle.vertex[1].x(), triangle.vertex[1].y(), triangle.vertex[1].z(), 1.0f);
            Vector4f v2 = mvp * Vector4f(triangle.vertex[2].x(), triangle.vertex[2].y(), triangle.vertex[2].z(), 1.0f);

            // 2. 改进的可见性检测（添加背面剔除和边界检查）
            if (!IsTriangleVisible(v0, v1, v2)) {
                return;
            }

            // 3. 透视除法
            v0 /= v0.w();
            v1 /= v1.w();
            v2 /= v2.w();

            // 4. 快速边界检查（替代精确的视口裁剪）
            if (!IsTriangleInBounds(v0, v1, v2)) {
                return;
            }

            // 5. Early Z-Rejection - 检查三角形最近深度
            // float nearestZ = std::min({v0.z(), v1.z(), v2.z()});
            // if (!CoarseDepthTest(v0, v1, v2, nearestZ)) {
            //     return; // 被粗略深度测试剔除
            // }

            // 6. 视口变换
            v0 = m_viewport * v0;
            v1 = m_viewport * v1;
            v2 = m_viewport * v2;

            // 7. 光栅化
            RasterizeTriangle(v0, v1, v2);
        }

        // 改进的可见性检测（添加背面剔除）
        bool IsTriangleVisible(const Vector4f& v0, const Vector4f& v1, const Vector4f& v2) {
            // 检查是否完全在近/远平面外
            bool allBehindNear = (v0.z() < -v0.w()) && (v1.z() < -v1.w()) && (v2.z() < -v2.w());
            bool allBeyondFar = (v0.z() > v0.w()) && (v1.z() > v1.w()) && (v2.z() > v2.w());
            
            if (allBehindNear || allBeyondFar) {
                return false;
            }

            // 背面剔除（在齐次空间中进行，避免除法）
            // 计算三角形法向量的 z 分量
            Vector3f e1(v1.x() - v0.x(), v1.y() - v0.y(), v1.z() - v0.z());
            Vector3f e2(v2.x() - v0.x(), v2.y() - v0.y(), v2.z() - v0.z());
            Vector3f normal = e1.cross(e2);
            
            // 如果法向量 z 分量为负，说明是背面（假设光源朝向 -z）
            if (normal.z() < 0) {
                return false;
            }

            return true;
        }

        // 快速边界检查（替代精确裁剪）
        bool IsTriangleInBounds(const Vector4f& v0, const Vector4f& v1, const Vector4f& v2) {
            // 计算三角形的 AABB
            float minX = std::min({v0.x(), v1.x(), v2.x()});
            float maxX = std::max({v0.x(), v1.x(), v2.x()});
            float minY = std::min({v0.y(), v1.y(), v2.y()});
            float maxY = std::max({v0.y(), v1.y(), v2.y()});
            
            // 检查 AABB 是否与 NDC 立方体 [-1,1]² 相交
            return !(maxX < -1.0f || minX > 1.0f || maxY < -1.0f || minY > 1.0f);
        }

        // 粗略深度测试 - Early Z-Rejection
        bool CoarseDepthTest(const Vector4f& v0, const Vector4f& v1, const Vector4f& v2, float nearestZ) {
            // 计算三角形在屏幕空间的边界框
            Vector4f sv0 = m_viewport * v0;
            Vector4f sv1 = m_viewport * v1;
            Vector4f sv2 = m_viewport * v2;
            
            int minX = std::max(0, (int)std::floor(std::min({sv0.x(), sv1.x(), sv2.x()}) / COARSE_FACTOR));
            int maxX = std::min(m_coarseWidth - 1, (int)std::ceil(std::max({sv0.x(), sv1.x(), sv2.x()}) / COARSE_FACTOR));
            int minY = std::max(0, (int)std::floor(std::min({sv0.y(), sv1.y(), sv2.y()}) / COARSE_FACTOR));
            int maxY = std::min(m_coarseHeight - 1, (int)std::ceil(std::max({sv0.y(), sv1.y(), sv2.y()}) / COARSE_FACTOR));
            
            // 检查粗略深度缓冲中是否有可能通过深度测试的像素
            for (int y = minY; y <= maxY; ++y) {
                for (int x = minX; x <= maxX; ++x) {
                    int coarseIdx = x + y * m_coarseWidth;
                    if (nearestZ < m_coarseDepthBuffer[coarseIdx]) {
                        return true; // 至少有一个像素可能通过测试
                    }
                }
            }
            
            return false; // 整个三角形都被遮挡
        }

        // 光栅化三角形（添加粗略深度缓冲更新）
        void RasterizeTriangle(const Vector4f& v0, const Vector4f& v1, const Vector4f& v2) {
            // 计算边界框
            int minX = std::max(0, (int)std::floor(std::min({v0.x(), v1.x(), v2.x()})));
            int maxX = std::min(m_width, (int)std::ceil(std::max({v0.x(), v1.x(), v2.x()})));
            int minY = std::max(0, (int)std::floor(std::min({v0.y(), v1.y(), v2.y()})));
            int maxY = std::min(m_height, (int)std::ceil(std::max({v0.y(), v1.y(), v2.y()})));

            // 记录深度更新，用于更新粗略深度缓冲
            bool depthUpdated = false;
            float minDepthUpdated = 1.0f;

            for (int y = minY; y < maxY; ++y) {
                for (int x = minX; x < maxX; ++x) {
                    if (InsideTriangle(x + 0.5f, y + 0.5f, v0, v1, v2)) {
                        // 计算重心坐标
                        auto [a, b, c] = Barycentric(x + 0.5f, y + 0.5f, 
                            Vector2f(v0.x(), v0.y()),
                            Vector2f(v1.x(), v1.y()),
                            Vector2f(v2.x(), v2.y()));

                        // 深度插值
                        float depth = v0.z() * a + v1.z() * b + v2.z() * c;
                        
                        // 深度测试并更新
                        int idx = x + (m_height - y - 1) * m_width;
                        if (depth < m_depthBuffer[idx]) {
                            m_depthBuffer[idx] = depth;
                            depthUpdated = true;
                            minDepthUpdated = std::min(minDepthUpdated, depth);
                        }
                    }
                }
            }

            // 更新粗略深度缓冲
            if (depthUpdated) {
                UpdateCoarseDepthBuffer(minX, maxX, minY, maxY, minDepthUpdated);
            }
        }

        // 更新粗略深度缓冲
        void UpdateCoarseDepthBuffer(int minX, int maxX, int minY, int maxY, float depth) {
            int coarseMinX = minX / COARSE_FACTOR;
            int coarseMaxX = std::min(m_coarseWidth - 1, maxX / COARSE_FACTOR);
            int coarseMinY = minY / COARSE_FACTOR;
            int coarseMaxY = std::min(m_coarseHeight - 1, maxY / COARSE_FACTOR);
            
            for (int y = coarseMinY; y <= coarseMaxY; ++y) {
                for (int x = coarseMinX; x <= coarseMaxX; ++x) {
                    int coarseIdx = x + y * m_coarseWidth;
                    m_coarseDepthBuffer[coarseIdx] = std::min(m_coarseDepthBuffer[coarseIdx], depth);
                }
            }
        }

        // 三角形内部测试
        bool InsideTriangle(float x, float y, const Vector4f& v0, const Vector4f& v1, const Vector4f& v2) {
            Vector2f p(x, y);
            Vector2f a(v0.x(), v0.y());
            Vector2f b(v1.x(), v1.y());
            Vector2f c(v2.x(), v2.y());

            Vector2f ab = b - a;
            Vector2f bc = c - b;
            Vector2f ca = a - c;

            Vector2f ap = p - a;
            Vector2f bp = p - b;
            Vector2f cp = p - c;

            float cross1 = ab.x() * ap.y() - ab.y() * ap.x();
            float cross2 = bc.x() * bp.y() - bc.y() * bp.x();
            float cross3 = ca.x() * cp.y() - ca.y() * cp.x();

            return (cross1 >= 0 && cross2 >= 0 && cross3 >= 0) ||
                   (cross1 <= 0 && cross2 <= 0 && cross3 <= 0);
        }

        // 重心坐标计算
        std::tuple<float, float, float> Barycentric(float x, float y, Vector2f A, Vector2f B, Vector2f C) {
            float i = (-(x - B.x()) * (C.y() - B.y()) + (y - B.y()) * (C.x() - B.x())) /
                    (-(A.x() - B.x()) * (C.y() - B.y()) + (A.y() - B.y()) * (C.x() - B.x()));
            float j = (-(x - C.x()) * (A.y() - C.y()) + (y - C.y()) * (A.x() - C.x())) /
                    (-(B.x() - C.x()) * (A.y() - C.y()) + (B.y() - C.y()) * (A.x() - C.x()));
            float k = 1 - i - j;
            return {i, j, k};
        }
    };
}