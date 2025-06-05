#pragma once

#include "CommonHeader.hpp"

#include "Model.hpp"
#include "Shape.hpp"
#include "Camera.hpp"
#include "Raster.hpp"

#include "Shaders/ShaderRegister.hpp"
#include "Materials/Material.hpp"

#include <omp.h>
#include <boost/pfr.hpp>

using namespace aries::shader;
using namespace aries::material;
using namespace aries::scene;

namespace aries::render {

    template<ShaderConcept ShaderT>
    struct PipelineFragmentData {
        ShaderT::FragmentPaylod_t fragmentPayload; // 片元着色器输入数据
        ShaderT::v2f_t fragmentData[3];
        float clipW[3];  // 保存透视除法前的 w 值
    };

    class Renderer {
    private:
        int _width, _height;

        Matrix4f _viewport;

        //std::vector<Vector3f> _frameBuffer;
        vector<float> _zBuffer;
        sptr<Raster> m_raster;
        sptr<Camera> m_camera;
        sptr<Scene> m_scene;

    public:
        Renderer() = delete;

        Renderer(sptr<Raster> raster, int w, int h);

        void SetCameraAndScene(sptr<Camera> camera, sptr<Scene> scene);

        void Clear(); // 清除缓存

        // 获取像素索引
        inline int GetPixelIndex(int x, int y) {
            return x + (_height - y - 1) * _width;
        }

        static Matrix4f GetModelMatrix(const Model& o);

        Matrix4f GetViewMatrix();

        Matrix4f GetClipMatrix();

        // 绘制一条3D线段
        void DrawLine3D(Vector3f start, Vector3f end, Vector3f color, bool ignoreDepthTest = false);

        // 绘制坐标系
        void DrawCoordinateSystem(float axisLength = 3.0f, bool showGrid = true, float gridSize = 0.1f, int gridCount = 20);

        template<ShaderConcept ShaderT> // 顶点着色器
        vector<PipelineFragmentData<ShaderT>> VertexShaderWith(vector<sptr<Shape>>& shapeList, uint64_t& triangleCount) { 
            static uint64_t lastTriangleCount = 0;

            // a2v → v2f → 组装 TriangleData 列表
            vector<PipelineFragmentData<ShaderT>> prims;
            prims.reserve(lastTriangleCount * 1.2f); // 预分配空间，避免频繁扩容，实测能加速顶点着色速度很多

            Matrix4f mat_model_to_view = GetViewMatrix();
            Matrix4f mat_view_to_clip = GetClipMatrix();

            for (auto shape : shapeList) {
                Matrix4f mat_view = mat_model_to_view * GetModelMatrix(*shape->model);

                auto shaderPropertyPtr = &static_cast<MaterialBase<ShaderT>*>(shape->material.get())->property;

                // 构造顶点着色器 Payload
                VertexPaylod<ShaderT> vp {
                    .camera = m_camera.get(),
                    .mat_view = mat_view,
                    .mat_mvp = mat_view_to_clip * mat_view, 
                    .property = shaderPropertyPtr
                };

                // 每个三角形
                for (size_t ti = 0; ti < shape->mesh.size(); ++ti) {
                    auto& tri = shape->mesh[ti];
                    
                    a2v in[3];
                    for (int k = 0; k < 3; ++k) {
                        in[k].position = Vector4f(tri.vertex[k].x(), tri.vertex[k].y(), tri.vertex[k].z(), 1.f);
                        in[k].normal = tri.normal[k];
                        in[k].uv = tri.uv[k];
                    }

                    // 装配到 TriangleData
                    PipelineFragmentData<ShaderT> pd;
                    pd.fragmentPayload = {
                        .scene = m_scene.get(),
                        .camera = m_camera.get(),
                        .mat_view = mat_view,
                        .property = shaderPropertyPtr
                    };

                    //* 调用 Shader::VertexShader 得到 v2f
                    pd.fragmentData[0] = ShaderBase<ShaderT>::VertexShader(vp, in[0]);
                    pd.fragmentData[1] = ShaderBase<ShaderT>::VertexShader(vp, in[1]);
                    pd.fragmentData[2] = ShaderBase<ShaderT>::VertexShader(vp, in[2]);
                    // 此时已经在NDC坐标系下，但是未经透视除法处理，先做裁剪再做透视除法

                    //* 近远平面裁剪
                    //? 为什么要先做裁剪，再做透视除法?
                    //? 1.第一个原因，避免裁剪出来的新三角形有畸变
                    //? 2. 进行透视除法之前会进行裁剪，会把z=0的部分剔除掉，从而保证透视除法的时候不会存在z=0的顶点。

                    bool allBehindNear = true, allBeyondFar = true, hasBehindNear = false, hasBeyondFar = false;
                    for (int i = 0; i < 3; ++i) {
                        if (pd.fragmentData[i].screenPos.z() >= -pd.fragmentData[i].screenPos.w())
                            allBehindNear = false;
                        else
                            hasBehindNear = true;
                        if (pd.fragmentData[i].screenPos.z() <= pd.fragmentData[i].screenPos.w())
                            allBeyondFar = false;
                        else
                            hasBeyondFar = true;
                    }
                    if (allBehindNear || allBeyondFar) {
                        continue; // 丢弃该三角形
                    }

                    // 卡在远平面间的三角形保留不裁剪，只裁近平面

                    if (hasBehindNear && !allBehindNear) [[unlikely]] {
                        // 有部分顶点在近平面后面，有部分在前面，需要裁剪
                        //* 收集裁剪后的顶点
                        //? 使用栈分配的固定大小数组替代 vector
                        typename ShaderT::v2f_t clippedVertices[5]; // 最多5个顶点
                        float clippedW[5];
                        int vertexCount = 0;

                        for (int i = 0; i < 3; ++i) {
                            int next = (i + 1) % 3;
                            
                            auto& current = pd.fragmentData[i];
                            auto& nextVertex = pd.fragmentData[next];
                            
                            // 检查当前顶点是否在近平面前面
                            bool currentInside = current.screenPos.z() >= -current.screenPos.w();
                            bool nextInside = nextVertex.screenPos.z() >= -nextVertex.screenPos.w();
                            
                            if (currentInside) {
                                // 当前顶点在近平面前面，添加它
                                clippedVertices[vertexCount] = current;
                                clippedW[vertexCount] = current.screenPos.w();
                                vertexCount++;
                                
                                if (!nextInside) {
                                    // 计算交点
                                    float t = ComputeNearPlaneIntersection(current.screenPos, nextVertex.screenPos);
                                    auto intersection = LinerInterpolateV2f<ShaderT>(current, nextVertex, t);
                                    clippedVertices[vertexCount] = intersection;
                                    clippedW[vertexCount] = intersection.screenPos.w();
                                    vertexCount++;
                                }
                            } else if (nextInside) {
                                // 计算交点
                                float t = ComputeNearPlaneIntersection(current.screenPos, nextVertex.screenPos);
                                auto intersection = LinerInterpolateV2f<ShaderT>(current, nextVertex, t);
                                clippedVertices[vertexCount] = intersection;
                                clippedW[vertexCount] = intersection.screenPos.w();
                                vertexCount++;
                            }
                        }
                        
                        // 如果裁剪后顶点数量不足3个，丢弃该三角形
                        if (vertexCount < 3) {
                            continue;
                        }
                        
                        //* 使用扇形三角剖分将裁剪后的多边形分解成三角形
                        for (int k = 1; k < vertexCount - 1; ++k) {
                            PipelineFragmentData<ShaderT> clippedPd;
                            clippedPd.fragmentPayload = pd.fragmentPayload;
                            
                            //* 设置三角形的三个顶点
                            clippedPd.fragmentData[0] = clippedVertices[0];
                            clippedPd.fragmentData[1] = clippedVertices[k];
                            clippedPd.fragmentData[2] = clippedVertices[k + 1];
                            
                            //* 设置对应的 w 值
                            clippedPd.clipW[0] = clippedW[0];
                            clippedPd.clipW[1] = clippedW[k];
                            clippedPd.clipW[2] = clippedW[k + 1];
                            
                            //* 透视除法
                            clippedPd.fragmentData[0].screenPos /= clippedPd.fragmentData[0].screenPos.w();
                            clippedPd.fragmentData[1].screenPos /= clippedPd.fragmentData[1].screenPos.w();
                            clippedPd.fragmentData[2].screenPos /= clippedPd.fragmentData[2].screenPos.w();

                            //* 视口裁剪
                            {
                                // 计算三角形的边界盒
                                float minX = std::min({clippedPd.fragmentData[0].screenPos.x(),
                                                    clippedPd.fragmentData[1].screenPos.x(),
                                                    clippedPd.fragmentData[2].screenPos.x()});
                                float maxX = std::max({clippedPd.fragmentData[0].screenPos.x(),
                                                    clippedPd.fragmentData[1].screenPos.x(),
                                                    clippedPd.fragmentData[2].screenPos.x()});
                                float minY = std::min({clippedPd.fragmentData[0].screenPos.y(),
                                                    clippedPd.fragmentData[1].screenPos.y(),
                                                    clippedPd.fragmentData[2].screenPos.y()});
                                float maxY = std::max({clippedPd.fragmentData[0].screenPos.y(),
                                                    clippedPd.fragmentData[1].screenPos.y(),
                                                    clippedPd.fragmentData[2].screenPos.y()});
                                
                                // 检查边界盒是否与视口相交
                                if (maxX < -1.0f || minX > 1.0f || maxY < -1.0f || minY > 1.0f) {
                                    continue; // 三角形边界盒与视口不相交，丢弃
                                }
                            }
                            
                            //* 背面剔除
                            {
                                Vector2f v0 = clippedPd.fragmentData[0].screenPos.template head<2>();
                                Vector2f v1 = clippedPd.fragmentData[1].screenPos.template head<2>();
                                Vector2f v2 = clippedPd.fragmentData[2].screenPos.template head<2>();
                                
                                Vector2f e1 = v1 - v0;
                                Vector2f e2 = v2 - v0;
                                
                                float crossZ = e1.x() * e2.y() - e1.y() * e2.x();
                                
                                if (crossZ < 0) {
                                    continue; // 丢弃背面三角形
                                }
                            }
                            
                            //* 视口变换
                            clippedPd.fragmentData[0].screenPos = _viewport * clippedPd.fragmentData[0].screenPos;
                            clippedPd.fragmentData[1].screenPos = _viewport * clippedPd.fragmentData[1].screenPos;
                            clippedPd.fragmentData[2].screenPos = _viewport * clippedPd.fragmentData[2].screenPos;
                            
                            // 添加到结果列表
                            prims.emplace_back(std::move(clippedPd));
                        }
                    } else [[likely]] {
                        // 所有顶点都在近平面前面，正常处理无需裁剪

                        //* 保存齐次坐标 w 分量，为后面透视矫正插值准备
                        pd.clipW[0] = pd.fragmentData[0].screenPos.w();
                        pd.clipW[1] = pd.fragmentData[1].screenPos.w();
                        pd.clipW[2] = pd.fragmentData[2].screenPos.w();

                        //* 齐次除法
                        // NDC坐标系 z ∈ [-1, 1]，靠近近平面时 z < 0，靠近远平面时 z > 0
                        pd.fragmentData[0].screenPos /= pd.fragmentData[0].screenPos.w(); 
                        pd.fragmentData[1].screenPos /= pd.fragmentData[1].screenPos.w();
                        pd.fragmentData[2].screenPos /= pd.fragmentData[2].screenPos.w();

                        //* 视口裁剪
                        {
                            // 计算三角形的边界盒
                            float minX = std::min({pd.fragmentData[0].screenPos.x(),
                                                pd.fragmentData[1].screenPos.x(),
                                                pd.fragmentData[2].screenPos.x()});
                            float maxX = std::max({pd.fragmentData[0].screenPos.x(),
                                                pd.fragmentData[1].screenPos.x(),
                                                pd.fragmentData[2].screenPos.x()});
                            float minY = std::min({pd.fragmentData[0].screenPos.y(),
                                                pd.fragmentData[1].screenPos.y(),
                                                pd.fragmentData[2].screenPos.y()});
                            float maxY = std::max({pd.fragmentData[0].screenPos.y(),
                                                pd.fragmentData[1].screenPos.y(),
                                                pd.fragmentData[2].screenPos.y()});
                            
                            // 检查边界盒是否与视口相交
                            if (maxX < -1.0f || minX > 1.0f || maxY < -1.0f || minY > 1.0f) {
                                continue; // 三角形边界盒与视口不相交，丢弃
                            }
                        }

                        //* 背面剔除
                        {
                            Vector2f v0 = pd.fragmentData[0].screenPos.template head<2>();
                            Vector2f v1 = pd.fragmentData[1].screenPos.template head<2>();
                            Vector2f v2 = pd.fragmentData[2].screenPos.template head<2>();
                            
                            // 计算两条边的向量
                            Vector2f e1 = v1 - v0;
                            Vector2f e2 = v2 - v0;
                            
                            // 计算叉积（2D向量的叉积实际是行列式）
                            float crossZ = e1.x() * e2.y() - e1.y() * e2.x();
                            
                            // 这里假设 crossZ < 0 表示背面
                            if (crossZ < 0) {
                                continue; // 丢弃背面三角形
                            }
                        }

                        //* 视口变换
                        pd.fragmentData[0].screenPos = _viewport * pd.fragmentData[0].screenPos; // 屏幕空间
                        pd.fragmentData[1].screenPos = _viewport * pd.fragmentData[1].screenPos; // 屏幕空间
                        pd.fragmentData[2].screenPos = _viewport * pd.fragmentData[2].screenPos; // 屏幕空间

                        // 将处理后的数据添加到片元列表
                        prims.emplace_back(std::move(pd));
                    }
                }
            }

            triangleCount += lastTriangleCount = prims.size(); // 更新最后处理的三角形数量
            return prims;
        }

        // 片元着色器(使用特定着色器类型)
        template<ShaderConcept ShaderT>
        void FragmentShaderWith(vector<PipelineFragmentData<ShaderT>>&& frags) {
#pragma omp parallel for schedule(static)
            for (size_t i = 0; i < frags.size(); ++i) {
                auto&& [paylod, frag, clipW] = frags[i];
                float minXf = _width, maxXf = 0, minYf = _height, maxYf = 0;
                for (int j = 0; j < 3; ++j) {
                    Vector4f v = frag[j].screenPos;
                    minXf = std::min(minXf, v.x());
                    maxXf = std::max(maxXf, v.x());
                    minYf = std::min(minYf, v.y());
                    maxYf = std::max(maxYf, v.y());
                }
                minXf = std::clamp(minXf, 0.0f, (float)_width);
                maxXf = std::clamp(maxXf, 0.0f, (float)_width);
                minYf = std::clamp(minYf, 0.0f, (float)_height);
                maxYf = std::clamp(maxYf, 0.0f, (float)_height);
                int minX = (int)std::floor(minXf), maxX = (int)std::ceil(maxXf);
                int minY = (int)std::floor(minYf), maxY = (int)std::ceil(maxYf);

//#pragma omp parallel for collapse(2) schedule(static)
                typename ShaderT::v2f_t v2f;
                for (int y = minY; y < maxY; ++y) {
                    for (int x = minX; x < maxX; ++x) {
                        // 判断像素是否在三角形内
                        if (InsideTriangle((float)x + 0.5f, (float)y + 0.5f,
                                        frag[0].screenPos.template head<3>(),
                                        frag[1].screenPos.template head<3>(),
                                        frag[2].screenPos.template head<3>()
                                    )) {
                            //* 计算2D重心坐标
                            auto [a, b, c] = 
                                Barycentric((float)x + 0.5f, (float)y + 0.5f,
                                    frag[0].screenPos.template head<2>(),
                                    frag[1].screenPos.template head<2>(),
                                    frag[2].screenPos.template head<2>()
                                );

                            //* 判断深度值
                            float theZ = frag[0].screenPos.z() * a + frag[1].screenPos.z() * b + frag[2].screenPos.z() * c; //? 深度值本来就算NDC空间的，所以不用透视插值

                            int idx = GetPixelIndex(x, y);

                            if (_zBuffer[idx] > theZ) {
                                _zBuffer[idx] = theZ;
                            } else {
                                continue; // 深度测试失败，跳过该像素
                            }
                            
                            //* 进行插值
                            {
                                v2f.screenPos = Vector4f(
                                    (float)x + 0.5f, 
                                    (float)y + 0.5f, 
                                    theZ, 
                                    1.0f
                                ); // 屏幕空间坐标，第一个字段单独插值

                                
                                // 透视校正插值
                                float invW[3] = {
                                    1.0f / clipW[0],
                                    1.0f / clipW[1],
                                    1.0f / clipW[2]
                                };

                                float interpInvW = a * invW[0] + b * invW[1] + c * invW[2];

                                auto Interpolate = [a, b, c, invW, interpInvW]<typename T>(T v0, T v1, T v2) -> T {
                                    return (v0 * a * invW[0] + 
                                            v1 * b * invW[1] + 
                                            v2 * c * invW[2]) / interpInvW;
                                };

                                constexpr size_t v2fSize = boost::pfr::tuple_size_v<decltype(v2f)> - 1; // 获取 v2f 余下的字段数量

                                //? 这里使用了C++20的折叠表达式和索引序列来实现编译期展开
                                //? 这样可以避免手动写每个字段的插值代码，提高可维护性
                                [&]<size_t... Is>(std::index_sequence<Is...>) -> void {
                                    ((
                                        boost::pfr::get<Is + 1>(v2f) = Interpolate(
                                            boost::pfr::get<Is + 1>(frag[0]),
                                            boost::pfr::get<Is + 1>(frag[1]),
                                            boost::pfr::get<Is + 1>(frag[2])
                                        )
                                    ), ...);
                                } (std::make_index_sequence<v2fSize>());
                            }

                            //* 使用shader处理着色
                            Vector3f pixelColor = ShaderBase<ShaderT>::FragmentShader(paylod, v2f);

                            
                            if (_zBuffer[idx] == theZ) [[likely]] { // 校验，防止多线程着色错误
                                SetPixelColor(x, y, pixelColor);
                            }
                        }
                    }
                }
            }
        }

    private:
        inline static bool InsideTriangle(float x, float y, Vector3f v0, Vector3f v1, Vector3f v2) {
            v0.z() = v1.z() = v2.z() = 1.0f;

            Vector3f p(x, y, 1);

            // 三条边的向量
            Vector3f side1, side2, side3;
            side1 = v1 - v0;
            side2 = v2 - v1;
            side3 = v0 - v2;

            // 顶点到点的向量
            Vector3f p1, p2, p3;
            p1 = p - v0;
            p2 = p - v1;
            p3 = p - v2;

            // 叉乘
            Vector3f cross1, cross2, cross3;
            cross1 = p1.cross(side1);
            cross2 = p2.cross(side2);
            cross3 = p3.cross(side3);

            // 判断是否同号
            if ((cross1.z() > 0 && cross2.z() > 0 && cross3.z() > 0) ||
                (cross1.z() < 0 && cross2.z() < 0 && cross3.z() < 0)) {
                return true;
            } else
                return false;
        }

        // 计算与近平面的交点参数 t
        inline static float ComputeNearPlaneIntersection(const Vector4f& p1, const Vector4f& p2) {
            // 近平面条件：z >= -w，即 z + w >= 0
            float d1 = p1.z() + p1.w();
            float d2 = p2.z() + p2.w();
            
            // 计算交点参数 t，使得 lerp(p1, p2, t) 刚好在近平面上
            return d1 / (d1 - d2);
        }
        
        // 线性插值两个 v2f 结构体
        template<ShaderConcept ShaderT>
        inline static typename ShaderT::v2f_t LinerInterpolateV2f(const typename ShaderT::v2f_t& v1, const typename ShaderT::v2f_t& v2, float t) {
            typename ShaderT::v2f_t result;

            // 使自动插值所有字段
            constexpr size_t fieldCount = boost::pfr::tuple_size_v<typename ShaderT::v2f_t>;
            
            [&]<size_t... Is>(std::index_sequence<Is...>) {
                ((boost::pfr::get<Is>(result) = 
                    boost::pfr::get<Is>(v1) * (1.0f - t) + 
                    boost::pfr::get<Is>(v2) * t), ...);
            } (std::make_index_sequence<fieldCount>());

            return result;
        }

        // 计算重心坐标
        inline static std::tuple<float, float, float> Barycentric(float x, float y, Vector2f A, Vector2f B, Vector2f C) {
            float i = (-(x - B.x()) * (C.y() - B.y()) + (y - B.y()) * (C.x() - B.x())) /
                    (-(A.x() - B.x()) * (C.y() - B.y()) + (A.y() - B.y()) * (C.x() - B.x()));
            float j = (-(x - C.x()) * (A.y() - C.y()) + (y - C.y()) * (A.x() - C.x())) /
                    (-(B.x() - C.x()) * (A.y() - C.y()) + (B.y() - C.y()) * (A.x() - C.x()));
            float k = 1 - i - j;
            return {i, j, k};
        }

        inline void SetPixelColor(int x,int y, const Vector3f color) { // 使颜色存入帧缓冲
            m_raster->SetPixel(x, y, color.x() * 255, color.y() * 255, color.z() * 255);
        }

        // 编译期着色器分派
        template<typename ShaderList>
        struct ShaderDispatcher;

    public:
        // 使用目标着色器类型渲染
        void RenderWithShader(ShaderType type, vector<sptr<Shape>>& shapeList, uint64_t& triangleCount);
    };

    // 特化 - 递归处理类型列表
    template<ShaderConcept First, ShaderConcept... Rest>
    struct Renderer::ShaderDispatcher<TypeList<First, Rest...>> {
        template<typename... Args>
        inline static void Dispatch(Renderer& self, ShaderType type, Args&&... args) {
            if (ShaderBase<First>::GetType() == type) {
                // 类型匹配，执行渲染
                auto prims = self.VertexShaderWith<First>(args...);
                self.FragmentShaderWith<First>(std::move(prims));
            } else {
                if constexpr (sizeof...(Rest) > 0) {
                    // 继续递归处理下一个类型
                    ShaderDispatcher<TypeList<Rest...>>::Dispatch(self, type, std::forward<Args>(args)...);
                } else {
                    // 没有匹配的类型，输出错误信息
                    std::cerr << "[Renderer::ShaderDispatcher] 未找到匹配的着色器类型: " << static_cast<int>(type) << std::endl;
                }
            }
        }
    };
}