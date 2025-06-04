#pragma once

#include <Eigen>
#include <numbers>

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

	Vector3f eluaAngle = Vector3f(0.f, 0.f, 0.f); // 欧拉角，单位为度
	// 这里的欧拉角定义为：
	// Yaw: 绕Y轴旋转（水平角度）
	// Pitch: 绕X轴旋转（垂直角度）
	// Roll: 绕Z轴旋转（翻滚角度）

	inline void ApplyEluaAngle() {
		const float M_PI = std::numbers::pi_v<float>;
		// 将欧拉角转换为方向向量
		float yawRad   = eluaAngle.x() * M_PI / 180.0f;
		float pitchRad = eluaAngle.y() * M_PI / 180.0f;
		float rollRad  = eluaAngle.z() * M_PI / 180.0f;

		Vector3f front;
		front.x() = cosf(pitchRad) * -sinf(yawRad);
		front.y() = sinf(pitchRad);
		front.z() = cosf(pitchRad) * -cosf(yawRad);
		Direction = front.normalized();

		Vector3f worldUp(0.0f, 1.0f, 0.0f);
		Vector3f right = Direction.cross(worldUp).normalized();
		Vector3f rolledRight = cosf(rollRad) * right + sinf(rollRad) * worldUp;
		Up = rolledRight.cross(Direction).normalized();
	}
};