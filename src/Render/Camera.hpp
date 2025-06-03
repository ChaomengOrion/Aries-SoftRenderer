#pragma once
#include <Eigen>

using Eigen::Vector2f;
using Eigen::Vector3f;
using Eigen::Vector4f;
using Eigen::Matrix4f;

struct Camera
{
	Vector4f Position;
	Vector3f Up;
	Vector3f Direction;
	float Fov;
	float Near; 
	float Far;
	float AspectRatio;
};