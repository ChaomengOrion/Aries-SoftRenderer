#pragma once

#include <Eigen>

using Eigen::Vector2f;
using Eigen::Vector3f;
using Eigen::Vector4f;
using Eigen::Matrix4f;

class Triangle
{
public:
    Vector3f vertex[3]; // 顶点
    Vector3f normal[3]; // 法线
    Vector2f uv[3]; // UV坐标

    Triangle(
        std::array<Vector3f, 3> vertices = {
            Vector3f::Zero(), 
            Vector3f::Zero(), 
            Vector3f::Zero()
        },
        std::array<Vector3f, 3> normals = {
            Vector3f(0, 0, 1), 
            Vector3f(0, 0, 1), 
            Vector3f(0, 0, 1)
        },
        std::array<Vector2f, 3> uvs = {
            Vector2f(0, 0), 
            Vector2f(0, 0), 
            Vector2f(0, 0)
        }
    )
    {
        SetVertex(vertices[0], vertices[1], vertices[2]);
        SetNormal(normals[0], normals[1], normals[2]);
        SetUV(uvs[0], uvs[1], uvs[2]);
    };

    void SetVertex(const Vector3f& v0, const Vector3f& v1, const Vector3f& v2)
    {
        vertex[0] = v0;
        vertex[1] = v1;
        vertex[2] = v2;
    }

    void SetVertex(int i, const Vector3f& ver)
    {
        vertex[i] = ver;
    }

    void SetNormal(const Vector3f& n0, const Vector3f& n1, const Vector3f& n2)
    {
        normal[0] = n0;
        normal[1] = n1;
        normal[2] = n2;
    }

    void SetNormal(int i, const Vector3f& n)
    {
        normal[i] = n;
    }

    void SetUV(const Vector2f& uv0, const Vector2f& uv1, const Vector2f& uv2)
    {
        uv[0] = uv0;
        uv[1] = uv1;
        uv[2] = uv2;
    }

    void SetUV(int i, const Vector2f& tC)
    {
        uv[i] = tC;
    }
};