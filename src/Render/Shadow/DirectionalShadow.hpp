#pragma once

#include "ShadowMapRenderer.hpp"

namespace aries::shadow {
    // 阴影采样结果结构
    struct ShadowSampleResult {
        float shadowFactor;      // 阴影因子 [0,1]，0=无阴影，1=完全阴影
        float occluderDistance;  // 到遮挡物的距离，用于衰减计算
        
        ShadowSampleResult(float shadow = 0.0f, float distance = 0.0f) 
            : shadowFactor(shadow), occluderDistance(distance) {}
    };

    class DirectionalShadow {
    private:
        std::unique_ptr<ShadowMapRenderer> m_shadowRenderer;
        Vector3f m_lightDirection;
        Vector3f m_lightPosition;
        float m_shadowBounds = 2.0f;  // 阴影范围
        float m_nearPlane = 0.1f;
        float m_farPlane = 10.0f;

    public:
        DirectionalShadow(const Vector3f& lightDir, int shadowMapSize = 2048) 
            : m_lightDirection(lightDir.normalized()) {
            m_shadowRenderer = std::make_unique<ShadowMapRenderer>(shadowMapSize);
            UpdateLightMatrices();
        }

        // 设置光源方向
        void SetLightDirection(const Vector3f& direction) {
            m_lightDirection = direction.normalized();
            UpdateLightMatrices();
        }

        // 设置阴影范围
        void SetShadowBounds(float bounds) {
            m_shadowBounds = bounds;
            UpdateLightMatrices();
        }

        // 设置近远平面
        void SetNearFarPlanes(float nearPlane, float farPlane) {
            m_nearPlane = nearPlane;
            m_farPlane = farPlane;
            UpdateLightMatrices();
        }

        // 更新阴影映射
        void UpdateShadowMap(vector<sptr<Shape>>& shapeList) {
            m_shadowRenderer->RenderShadowMap(shapeList);
        }

        // 简单阴影采样（带距离信息）
        ShadowSampleResult SampleShadowWithDistance(const Vector3f& worldPos, float bias = 0.005f) const {
            Matrix4f lightSpaceMatrix = GetLightViewProjectionMatrix();
            Vector4f lightSpacePos = lightSpaceMatrix * Vector4f(worldPos.x(), worldPos.y(), worldPos.z(), 1.0f);
            lightSpacePos /= lightSpacePos.w();

            float u = (lightSpacePos.x() + 1.0f) * 0.5f;
            float v = (lightSpacePos.y() + 1.0f) * 0.5f;
            float currentDepth = lightSpacePos.z();

            if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f || currentDepth > 1.0f) {
                return ShadowSampleResult(0.0f, 0.0f); // 超出范围，无阴影
            }

            float shadowDepth = m_shadowRenderer->SampleDepth(u, v);
            
            if (currentDepth > shadowDepth + bias) {
                // 在阴影中，计算到遮挡物的距离
                float occluderDistance = currentDepth - shadowDepth;
                return ShadowSampleResult(1.0f, occluderDistance);
            } else {
                // 不在阴影中
                return ShadowSampleResult(0.0f, 0.0f);
            }
        }

        // PCF 阴影采样（带距离信息）
        ShadowSampleResult SampleShadowPCFWithDistance(const Vector3f& worldPos, int pcfSize = 3, float bias = 0.005f) const {
            Matrix4f lightSpaceMatrix = GetLightViewProjectionMatrix();
            Vector4f lightSpacePos = lightSpaceMatrix * Vector4f(worldPos.x(), worldPos.y(), worldPos.z(), 1.0f);
            lightSpacePos /= lightSpacePos.w();

            float u = (lightSpacePos.x() + 1.0f) * 0.5f;
            float v = (lightSpacePos.y() + 1.0f) * 0.5f;
            float currentDepth = lightSpacePos.z();

            if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f || currentDepth > 1.0f) {
                return ShadowSampleResult(0.0f, 0.0f);
            }

            float totalShadow = 0.0f;
            float totalDistance = 0.0f;
            int sampleCount = 0;
            int shadowSamples = 0;

            float texelSize = 1.0f / m_shadowRenderer->GetWidth();
            int halfSize = pcfSize / 2;
            
            for (int y = -halfSize; y <= halfSize; ++y) {
                for (int x = -halfSize; x <= halfSize; ++x) {
                    float sampleU = u + x * texelSize;
                    float sampleV = v + y * texelSize;
                    
                    if (sampleU >= 0.0f && sampleU <= 1.0f && sampleV >= 0.0f && sampleV <= 1.0f) {
                        float shadowDepth = m_shadowRenderer->SampleDepth(sampleU, sampleV);
                        
                        if (currentDepth > shadowDepth + bias) {
                            // 在阴影中
                            totalShadow += 1.0f;
                            totalDistance += (currentDepth - shadowDepth);
                            shadowSamples++;
                        }
                        sampleCount++;
                    }
                }
            }
            
            if (sampleCount == 0) {
                return ShadowSampleResult(0.0f, 0.0f);
            }

            float shadowFactor = totalShadow / sampleCount;
            float avgDistance = (shadowSamples > 0) ? (totalDistance / shadowSamples) : 0.0f;
            
            return ShadowSampleResult(shadowFactor, avgDistance);
        }

        // 在世界坐标处采样阴影
        float SampleShadow(const Vector3f& worldPos, float bias = 0.005f) const {
            // 转换到光源空间
            Matrix4f lightSpaceMatrix = GetLightViewProjectionMatrix();
            Vector4f lightSpacePos = lightSpaceMatrix * Vector4f(worldPos.x(), worldPos.y(), worldPos.z(), 1.0f);
            
            // 透视除法
            lightSpacePos /= lightSpacePos.w();

            // 转换到纹理坐标 [0,1]
            float u = (lightSpacePos.x() + 1.0f) * 0.5f;
            float v = (lightSpacePos.y() + 1.0f) * 0.5f;
            float currentDepth = lightSpacePos.z();

            // 边界检查
            if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f || currentDepth > 1.0f) {
                return 0.0f; // 超出阴影映射范围，无阴影
            }

            // 采样阴影映射
            float shadowDepth = m_shadowRenderer->SampleDepth(u, v);
            
            // 阴影比较
            return (currentDepth - bias > shadowDepth) ? 1.0f : 0.0f;
        }

        // PCF 软阴影采样
        float SampleShadowPCF(const Vector3f& worldPos, int pcfSize = 3, float bias = 0.005f) const {
            Matrix4f lightSpaceMatrix = GetLightViewProjectionMatrix();
            Vector4f lightSpacePos = lightSpaceMatrix * Vector4f(worldPos.x(), worldPos.y(), worldPos.z(), 1.0f);
            lightSpacePos /= lightSpacePos.w();

            float u = (lightSpacePos.x() + 1.0f) * 0.5f;
            float v = (lightSpacePos.y() + 1.0f) * 0.5f;
            float currentDepth = lightSpacePos.z();

            if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f || currentDepth > 1.0f) {
                return 0.0f;
            }

            // PCF 采样
            float shadow = 0.0f;
            float texelSize = 1.0f / m_shadowRenderer->GetWidth();
            int halfSize = pcfSize / 2;
            
            for (int y = -halfSize; y <= halfSize; ++y) {
                for (int x = -halfSize; x <= halfSize; ++x) {
                    float sampleU = u + x * texelSize;
                    float sampleV = v + y * texelSize;
                    
                    if (sampleU >= 0.0f && sampleU <= 1.0f && sampleV >= 0.0f && sampleV <= 1.0f) {
                        float shadowDepth = m_shadowRenderer->SampleDepth(sampleU, sampleV);
                        shadow += (currentDepth - bias > shadowDepth) ? 1.0f : 0.0f;
                    }
                }
            }
            
            return shadow / (pcfSize * pcfSize);
        }

        // 获取光源视图投影矩阵
        Matrix4f GetLightViewProjectionMatrix() const {
            return m_shadowRenderer->GetLightViewProjectionMatrix();
        }

        // 获取阴影映射渲染器
        const ShadowMapRenderer* GetShadowRenderer() const {
            return m_shadowRenderer.get();
        }

        Vector3f GetLightDirection() const {
            return m_lightDirection;
        }

        Vector3f GetLightPosition() const {
            return m_lightPosition;
        }

        void SaveShadowMap(const std::string& filename) const {
            m_shadowRenderer->SaveDepthMapAsImage(filename);
        }
    private:
        void UpdateLightMatrices() {
            // 构建光源位置（在场景中心前方）
            m_lightPosition = -m_lightDirection * m_shadowBounds;
            Vector3f target = Vector3f::Zero();
            Vector3f up = Vector3f(0, 1, 0);
            
            // 避免up向量与光方向平行
            if (std::abs(m_lightDirection.dot(up)) > 0.99f) {
                up = Vector3f(1, 0, 0);
            }

            // 构建光源视图矩阵
            Vector3f forward = (target - m_lightPosition).normalized();
            Vector3f right = forward.cross(up).normalized();
            Vector3f camera_up = right.cross(forward);

            Matrix4f lightViewMatrix;
            lightViewMatrix <<
                right.x(),    right.y(),    right.z(),    -right.dot(m_lightPosition),
                camera_up.x(), camera_up.y(), camera_up.z(), -camera_up.dot(m_lightPosition),
                -forward.x(), -forward.y(), -forward.z(),  forward.dot(m_lightPosition),
                0,            0,            0,             1;

            // 构建正交投影矩阵
            float orthoSize = m_shadowBounds;
            Matrix4f lightProjectionMatrix;
            lightProjectionMatrix <<
                2.0f / (2 * orthoSize), 0,                     0,                           0,
                0,                      2.0f / (2 * orthoSize), 0,                           0,
                0,                      0,                     -2.0f / (m_farPlane - m_nearPlane), -(m_farPlane + m_nearPlane) / (m_farPlane - m_nearPlane),
                0,                      0,                     0,                           1;

            // 设置到阴影映射渲染器
            m_shadowRenderer->SetLightMatrices(lightViewMatrix, lightProjectionMatrix);
        }
    };
}