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

    class Renderer
    {
    private:
        int _width, _height;

        Matrix4f _viewport;

        //std::vector<Vector3f> _frameBuffer;
        vector<float> _zBuffer;
        sptr<Raster> m_raster;

    public:
        Renderer() = delete;
        
        Renderer(sptr<Raster> raster, int w, int h);

        void Clear(); // 清除缓存

        // 获取像素索引
        inline int GetPixelIndex(int x, int y) {
            return x + (_height - y - 1) * _width;
        }

        static Matrix4f GetModelMatrix(const Model& o);

        static Matrix4f GetViewMatrix(Camera& c);

        static Matrix4f GetClipMatrix(const Camera& c);

        template<ShaderConcept ShaderT> // 顶点着色器
        vector<PipelineFragmentData<ShaderT>> VertexShaderWith(vector<sptr<Shape>>& shapeList, sptr<Scene>& scene, sptr<Camera>& cam, uint64_t& triangleCount) { 
            static uint64_t lastTriangleCount = 0;

            // a2v → v2f → 组装 TriangleData 列表
            vector<PipelineFragmentData<ShaderT>> prims;
            prims.reserve(lastTriangleCount * 1.2f); // 预分配空间，避免频繁扩容，实测能加速顶点着色速度很多

            Matrix4f mat_v = GetViewMatrix(*cam);

            for (auto shape : shapeList) {
                Matrix4f view = mat_v * GetModelMatrix(*shape->model);

                auto shaderPropertyPtr = &static_cast<MaterialBase<ShaderT>*>(shape->material.get())->property;

                // 构造顶点着色器 Payload
                VertexPaylod<ShaderT> vp {
                    .camera = cam.get(),
                    .view = view,
                    .mvp = GetClipMatrix(*cam) * view, 
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
                        .scene = scene.get(),
                        .camera = cam.get(),
                        .view = view,
                        .property = shaderPropertyPtr
                    };

                    //* 调用 Shader::VertexShader 得到 v2f
                    pd.fragmentData[0] = ShaderBase<ShaderT>::VertexShader(vp, in[0]);
                    pd.fragmentData[1] = ShaderBase<ShaderT>::VertexShader(vp, in[1]);
                    pd.fragmentData[2] = ShaderBase<ShaderT>::VertexShader(vp, in[2]);

                    //* 裁剪
                    bool allBehindNear = true, allBeyondFar = true;
                    for (int i = 0; i < 3; ++i) {
                        if (pd.fragmentData[i].viewport.z() >= -pd.fragmentData[i].viewport.w())
                            allBehindNear = false;
                        if (pd.fragmentData[i].viewport.z() <= pd.fragmentData[i].viewport.w())
                            allBeyondFar = false;
                    }
                    if (allBehindNear || allBeyondFar) {
                        continue; // 丢弃该三角形
                    }

                    //* 保存齐次坐标 w 分量
                    pd.clipW[0] = pd.fragmentData[0].viewport.w();
                    pd.clipW[1] = pd.fragmentData[1].viewport.w();
                    pd.clipW[2] = pd.fragmentData[2].viewport.w();

                    //* 齐次除法
                    pd.fragmentData[0].viewport /= pd.fragmentData[0].viewport.w(); // NDC
                    pd.fragmentData[1].viewport /= pd.fragmentData[1].viewport.w(); // NDC
                    pd.fragmentData[2].viewport /= pd.fragmentData[2].viewport.w(); // NDC

                    //* 背面剔除
                    {
                        Vector2f v0 = pd.fragmentData[0].viewport.template head<2>();
                        Vector2f v1 = pd.fragmentData[1].viewport.template head<2>();
                        Vector2f v2 = pd.fragmentData[2].viewport.template head<2>();
                        
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
                    pd.fragmentData[0].viewport = _viewport * pd.fragmentData[0].viewport; // 屏幕空间
                    pd.fragmentData[1].viewport = _viewport * pd.fragmentData[1].viewport; // 屏幕空间
                    pd.fragmentData[2].viewport = _viewport * pd.fragmentData[2].viewport; // 屏幕空间

                    // 将处理后的数据添加到片元列表
                    prims.emplace_back(std::move(pd));
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
                    Vector4f v = frag[j].viewport;
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
                v2f<ShaderT> v2f;
                for (int y = minY; y < maxY; ++y) {
                    for (int x = minX; x < maxX; ++x) {
                        // 判断像素是否在三角形内
                        if (InsideTriangle((float)x + 0.5f, (float)y + 0.5f,
                                        frag[0].viewport.template head<3>(),
                                        frag[1].viewport.template head<3>(),
                                        frag[2].viewport.template head<3>()
                                    )) {
                            //* 计算2D重心坐标
                            auto [a, b, c] = 
                                Barycentric((float)x + 0.5f, (float)y + 0.5f,
                                    frag[0].viewport.template head<2>(),
                                    frag[1].viewport.template head<2>(),
                                    frag[2].viewport.template head<2>()
                                );

                            //* 透视校正插值
                            float invW[3] = {
                                1.0f / clipW[0],
                                1.0f / clipW[1],
                                1.0f / clipW[2]
                            };

                            float interpInvW = a * invW[0] + b * invW[1] + c * invW[2];

                            auto Interpolate = [a, b, c, invW, interpInvW]<typename T>(T v0, T v1, T v2) {
                                return (v0 * a * invW[0] + 
                                        v1 * b * invW[1] + 
                                        v2 * c * invW[2]) / interpInvW;
                            };

                            //* 判断深度值
                            float theZ = Interpolate(frag[0].viewport.z(), frag[1].viewport.z(), frag[2].viewport.z());

                            int idx = GetPixelIndex(x, y);

                            if (_zBuffer[idx] > theZ) {
                                _zBuffer[idx] = theZ;
                            } else {
                                continue; // 深度测试失败，跳过该像素
                            }
                            
                            //* 进行插值

                            {
                                constexpr size_t v2fSize = boost::pfr::tuple_size_v<decltype(v2f)>; // 获取v2f的字段数量

                                //? 这里使用了C++20的折叠表达式和索引序列来实现编译期展开
                                //? 这样可以避免手动写每个字段的插值代码，提高可维护性
                                [&]<size_t... Is>(std::index_sequence<Is...>) {
                                    ((
                                        boost::pfr::get<Is>(v2f) = Interpolate(
                                            boost::pfr::get<Is>(frag[0]),
                                            boost::pfr::get<Is>(frag[1]),
                                            boost::pfr::get<Is>(frag[2])
                                        )
                                    ), ...);
                                } (std::make_index_sequence<v2fSize>());
                            }

                            // 使用shader处理着色
                            Vector3f pixelColor = ShaderBase<ShaderT>::FragmentShader(paylod, v2f);

                            [[likely]]
                            if (_zBuffer[idx] == theZ) // 校验，防止多线程着色错误
                                SetPixelColor(x, y, pixelColor);
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
        void RenderWithShader(ShaderType type, vector<sptr<Shape>>& shapeList, sptr<Scene>& scene, sptr<Camera>& cam, uint64_t& triangleCount);
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