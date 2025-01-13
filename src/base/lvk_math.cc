#include "lvk_math.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/quaternion_geometric.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"

// math notes
// * 轴的顺序, 我们约定构成空间的V1, V2, V3三个轴分别是X, Y, Z.
// * 手性, 右手: cross(X, Y) == Z, 左手: cross(X, Y) = -Z, 这里使用右手.
// * 方向: 我们约定 Y == UP, X == LEFT, Z == FORWARD
// * 旋转的方向: 我们规定按轴的顺序旋转即为旋转的正方向, 比如围绕Z轴, X->Y方向旋转, 即旋转XY平面为正方向, 旋转YX平面为反方向.
// * 绕Y轴旋转: 根据右手定则, cross(Z, X) == Y, 所以绕Y轴时, Z->X方向为正方形, 由此推导出来的旋转矩阵, 跟其它2个轴, +-符号不同.
// * 欧拉角的应用顺序: 我们使用 Yaw-Pitch-Roll 的顺序应用欧拉角, 即构造这样的旋转矩阵: R(roll) * R(pitch) * R(yaw)
// * Model矩阵的构造: 我们按照 Scale->Rotate->Translate 的顺序构造Model矩阵

namespace lvk {

namespace matrix {

mat4f Scale(const mat4f& m, const vec3f& v) {
  mat4f result;
  result[0] = m[0] * v[0];
  result[1] = m[1] * v[1];
  result[2] = m[2] * v[2];
  result[3] = m[3];
  return result;
}

// m*v
mat4f Translate(const mat4f& m, const vec3f& v) {
  mat4f result(m);
  result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
  return result;
}

// 围绕 axis-v 旋转角度 angle 的旋转矩阵推导具体可以参考:
// see https://en.wikipedia.org/wiki/Rotation_matrix
mat4f Rotate(const mat4f& m, float angle, const vec3f& v) {
#ifdef USE_GLM
  return glm::rotate(m, glm::radians(angle), v);
#else
  float a = glm::radians(angle);
  float c = cos(a);
  float s = sin(a);
  vec3f axis = glm::normalize(v);
  vec3f temp = (1 - c) * axis;

  mat4f rot;

  rot[0][0] = c + temp[0] * axis[0];
  rot[0][1] = temp[0] * axis[1] + s * axis[2];
  rot[0][2] = temp[0] * axis[2] - s * axis[1];

  rot[1][0] = temp[1] * axis[0] - s * axis[2];
  rot[1][1] = c + temp[1] * axis[1];
  rot[1][2] = temp[1] * axis[2] + s * axis[0];

  rot[2][0] = temp[2] * axis[0] + s * axis[1];
  rot[2][1] = temp[2] * axis[1] - s * axis[0];
  rot[2][2] = c + temp[2] * axis[2];

  mat4f result;
  result[0] = m[0] * rot[0][0] + m[1] * rot[0][1] + m[2] * rot[0][2];
  result[1] = m[0] * rot[1][0] + m[1] * rot[1][1] + m[2] * rot[1][2];
  result[2] = m[0] * rot[2][0] + m[1] * rot[2][1] + m[2] * rot[2][2];
  result[3] = m[3];
  return result;
#endif
}

vec3f RotateVector(const vec3f& v, float angle, const vec3f& axis) {
#ifdef USE_GLM
  return glm::rotate(v, glm::radians(angle), axis);
#else
  return mat3f(Rotate(mat4f{1.0}, angle, axis)) * v;
#endif
}

mat4f MakeFromQuat(const vec4f& in_quat) {
  glm::quat quat = glm::quat(in_quat.w, in_quat.x, in_quat.y, in_quat.z);
  return glm::mat4_cast(quat);
}

mat4f MakeFromZ(const vec3f& z_axis_in) {
  vec3f z_axis = glm::normalize(z_axis_in);

  // 如果 z_axis.y 很接近1, 说明当前的 z_axis_in 跟 global.y 几乎平行
  // 此时我们就用 global.x 来叉乘
  vec3f up_vector = (abs(z_axis.y) < (1.0f - 0.0001)) ? vec3f(0, 1, 0) : vec3f(0, 0, 1);

  vec3f x_axis = glm::cross(up_vector, z_axis);
  vec3f y_axis = glm::cross(z_axis, x_axis);

  mat4f mat;
  mat[0] = vec4f(x_axis, 0);
  mat[1] = vec4f(y_axis, 0);
  mat[2] = vec4f(z_axis, 0);
  mat[3] = vec4f(0, 0, 0, 1);
  return mat;
}

mat4f MakeFromEulerAngleYXZ(const float& yaw, const float& pitch, const float& roll) {
#if 0
    // rotate 3 times
    mat4f rot{1};
    rot = Rotate(rot, yaw, vec3f(0, 1, 0));
    rot = Rotate(rot, pitch, vec3f(1, 0, 0));
    rot = Rotate(rot, roll, vec3f(0, 0, 1));
    return rot;
#endif

#if 1
    // 教科书一般的3个选择矩阵
    mat3f ry{1};
    float SinY = sin(yaw);
    float CosY = cos(yaw);
    ry[0][0] = CosY;
    ry[0][1] = 0;
    ry[0][2] = -SinY;
    ry[1][0] = 0;
    ry[1][1] = 1;
    ry[1][2] = 0;
    ry[2][0] = SinY;
    ry[2][1] = 0;
    ry[2][2] = CosY;


    mat3f rx{1};
    float SinX = sin(pitch);
    float CosX = cos(pitch);
    rx[0][0] = 1;
    rx[0][1] = 0;
    rx[0][2] = 0;
    rx[1][0] = 0;
    rx[1][1] = CosX;
    rx[1][2] = SinX;
    rx[2][0] = 0;
    rx[2][1] = -SinX;
    rx[2][2] = CosX;

    mat3f rz{1};
    float SinZ = sin(roll);
    float CosZ = cos(roll);
    rz[0][0] = CosZ;
    rz[0][1] = SinZ;
    rz[0][2] = 0;
    rz[1][0] = -SinZ;
    rz[1][1] = CosZ;
    rz[1][2] = 0;
    rz[2][0] = 0;
    rz[2][1] = 0;
    rz[2][2] = 1;

    mat4f result = mat4f(ry * rx * rz);
    result[3] = vec4f(0, 0, 0, 1);
    return result;
#endif

#if 0
    float SinY = sin(yaw);
    float CosY = cos(yaw);
    float SinX = sin(pitch);
    float CosX = cos(pitch);
    float SinZ = sin(roll);
    float CosZ = cos(roll);

    mat4f rot{1};
    rot[0][0] = CosX * CosZ;
    rot[0][1] = 0;
    rot[0][2] = 0;
    rot[1][0] = 0;
    rot[1][1] = 0;
    rot[1][2] = 0;
    rot[2][0] = 0;
    rot[2][1] = 0;
    rot[2][2] = 0;
#endif
}

mat4f MakeFromLookAt(const vec3f& start, const vec3f& target) { return MakeFromZ(target - start); }

vec3f DecomposeRotationFromMatrix(const mat4f& in_matrix) {
  const vec3f left = glm::normalize(vec3f(in_matrix[0]));     // Normalized left axis
  const vec3f up = glm::normalize(vec3f(in_matrix[1]));       // Normalized up axis
  const vec3f forward = glm::normalize(vec3f(in_matrix[2]));  // Normalized forward axis
  // Obtain the "unscaled" transform matrix
  mat4f m(0.0f);
  m[0][0] = left.x;
  m[0][1] = left.y;
  m[0][2] = left.z;

  m[1][0] = up.x;
  m[1][1] = up.y;
  m[1][2] = up.z;

  m[2][0] = forward.x;
  m[2][1] = forward.y;
  m[2][2] = forward.z;

  vec3f rot;
  rot.x = atan2f(m[1][2], m[2][2]);
  rot.y = atan2f(-m[0][2], sqrtf(m[1][2] * m[1][2] + m[2][2] * m[2][2]));
  rot.z = atan2f(m[0][1], m[0][0]);
  rot = glm::degrees(rot);
  return rot;
}

void DecomposeMatrix(const mat4f in_matrix, vec3f& out_translate, vec3f& out_scale, vec3f& out_rot) {
  out_translate = in_matrix[3];

  out_scale.x = glm::length(vec3f(in_matrix[0]));
  out_scale.y = glm::length(vec3f(in_matrix[1]));
  out_scale.z = glm::length(vec3f(in_matrix[2]));

  out_rot = DecomposeRotationFromMatrix(in_matrix);
}

mat4f ProjectionMatrix(float fov, float aspec, float near, float far) {
  return mat4f{0};
}

#if 0
template<typename T>
	GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> perspectiveRH_ZO(T fovy, T aspect, T zNear, T zFar)
	{
		assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

		T const tanHalfFovy = tan(fovy / static_cast<T>(2));

		mat<4, 4, T, defaultp> Result(static_cast<T>(0));
		Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
		Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
		Result[2][2] = zFar / (zNear - zFar);
		Result[2][3] = - static_cast<T>(1);
		Result[3][2] = -(zFar * zNear) / (zFar - zNear);
		return Result;
	}
#endif
}  // namespace matrix

}  // namespace lvk