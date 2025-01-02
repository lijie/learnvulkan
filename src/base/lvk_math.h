#pragma once

#include <glm/gtx/string_cast.hpp>

#include "glm/ext/matrix_float4x4.hpp"


#undef USE_GLM

namespace lvk {

using vec3f = glm::vec3;
using vec4f = glm::vec4;
using vec2f = glm::vec2;
using mat3f = glm::mat3x3;
using mat4f = glm::mat4x4;

constexpr vec3f ZERO_VECTOR = vec3f(0.0, 0.0, 0.0);

namespace vector {
constexpr vec3f UP = vec3f(0.0, 1.0, 0.0);
constexpr vec3f LEFT = vec3f(1.0, 0.0, 0.0);
constexpr vec3f RIGHT = vec3f(-1.0, 0.0, 0.0);
constexpr vec3f FORWARD = vec3f(0.0, 0.0, 1.0);

}  // namespace vector

namespace matrix {

mat4f Scale(const mat4f& m, const vec3f& v);
mat4f Translate(const mat4f& m, const vec3f& v);
mat4f Rotate(const mat4f& m, float angle, const vec3f& v);

vec3f RotateVector(const vec3f& v, float angle, const vec3f& axis);

mat4f MakeFromQuat(const vec4f& in_quat);
// 通过 forward 轴构造旋转矩阵
mat4f MakeFromZ(const vec3f& z_axis);
// 通过LookAt构造旋转矩阵
mat4f MakeFromLookAt(const vec3f& start, const vec3f& target);
// 通过欧拉角构造旋转矩阵
mat4f MakeFromEulerAngleYXZ(const float& yaw, const float& pitch, const float& roll);

// 从 4x4 矩阵中分解 Rotation
vec3f DecomposeRotationFromMatrix(const mat4f& in_matrix);
// 从 4x4 矩阵中分解 translation, scale & rotation
void DecomposeMatrix(const mat4f in_matrix, vec3f& out_translate, vec3f& out_scale, vec3f& out_rot);
}  // namespace matrix

}  // namespace lvk
