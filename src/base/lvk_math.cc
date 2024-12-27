#include "lvk_math.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/quaternion_geometric.hpp"
#include "glm/geometric.hpp"
#include <glm/gtc/quaternion.hpp>

namespace lvk {

namespace matrix {

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

}  // namespace matrix

}  // namespace lvk