#pragma once
#include <memory>
#include <Eigen>
#include <utility>
#include <iostream>
#include <numbers>

using Eigen::Vector2f;
using Eigen::Vector3f;
using Eigen::Vector4f;
using Eigen::Matrix4f;

using std::vector;
using std::string;

template<typename T>
using sptr = std::shared_ptr<T>;

template<typename T>
using uptr = std::unique_ptr<T>;


inline constexpr float ToRadian(float angle) {
    return (angle / 180) * std::numbers::pi_v<float>;
}
