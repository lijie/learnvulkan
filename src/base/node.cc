#include "base/node.h"

#include "glm/ext/matrix_transform.hpp"
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include "glm/trigonometric.hpp"
#include <glm/gtc/epsilon.hpp>
#include "lvk_log.h"
#include "lvk_math.h"

namespace lvk {

Node::Node() {}

mat4f Node::localMatrix() {
#if 1
  matrix = mat4f(1.0);
  // matrix = glm::translate(mat4f{1.0}, transform.translation * vec3f(1, 1, 1));
  // matrix = glm::rotate(matrix, glm::radians(transform.rotation.x), vec3f(1, 0, 0));
  // matrix = glm::rotate(matrix, glm::radians(transform.rotation.y), vec3f(0, 1, 0));
  // matrix = glm::rotate(matrix, glm::radians(transform.rotation.z), vec3f(0, 0, 1));
  matrix = glm::translate(mat4f{1.0}, transform.translation * vec3f(1, 1, 1));
  auto rm = glm::eulerAngleYXZ(glm::radians(transform.rotation.y), glm::radians(transform.rotation.x), glm::radians(transform.rotation.z));
  matrix = matrix * rm;
  // glm::quat quat(vec3f(glm::radians(transform.rotation.x), glm::radians(transform.rotation.y), glm::radians(transform.rotation.z)));
  // matrix = matrix * glm::toMat4(quat);

#if 0
  glm::quat quat(rm);
  if (glm::epsilonNotEqual(transform.rotation.x, glm::degrees(glm::pitch(quat)), 0.01f)) {
    DEBUG_LOG("rotation x: {}, {}", transform.rotation.x, glm::degrees(glm::pitch(quat)));
  }
  if (glm::epsilonNotEqual(transform.rotation.y, glm::degrees(glm::yaw(quat)), 0.01f)) {
    DEBUG_LOG("rotation y: {}, {}", transform.rotation.y, glm::degrees(glm::yaw(quat)));
  }
  if (glm::epsilonNotEqual(transform.rotation.z, glm::degrees(glm::roll(quat)), 0.01f)) {
    DEBUG_LOG("rotation z: {}, {}", transform.rotation.z, glm::degrees(glm::roll(quat)));
  }

  transform.rotation.x = glm::degrees(glm::pitch(quat));
  transform.rotation.y = glm::degrees(glm::yaw(quat));
  transform.rotation.z = glm::degrees(glm::roll(quat));
#endif
#else

  // 这里不考虑效率, 让计算过程清晰易懂.

  matrix = glm::identity<mat4f>();

  float SX = sin(glm::radians(transform.rotation.x));
  float CX = cos(glm::radians(transform.rotation.x));

  mat4f RMX = {
      {1.0f, 0.0f, 0.0f, 0.0f},
      {0.0f, CX, SX, 0.0f},
      {0.0f, -SX, CX, 0.0f},
      {0.0f, 0.0f, 0.0f, 1.0f},
  };

  // DEBUG_LOG("RMX: {}", glm::to_string(RMX));
  // DEBUG_LOG("apply RMX: {}", glm::to_string(RMX * matrix));

  float SY = sin(glm::radians(transform.rotation.y));
  float CY = cos(glm::radians(transform.rotation.y));

  mat4f RMY = {
      {CY, 0.0f, -SY, 0.0f},
      {0.0f, 1.0f, 0.0f, 0.0f},
      {SY, 0.0f, CY, 0.0f},
      {0.0f, 0.0f, 0.0f, 1.0f},
  };

  float SZ = sin(glm::radians(transform.rotation.z));
  float CZ = cos(glm::radians(transform.rotation.z));

  mat4f RMZ = {
      {CZ, SZ, 0.0f, 0.0f},
      {-SZ, CZ, 0.0f, 0.0f},
      {0.0f, 0.0f, 1.0f, 0.0f},
      {0.0f, 0.0f, 0.0f, 1.0f},
  };

  matrix = RMZ * RMY * RMX * matrix;

  matrix[3][0] = transform.translation.x;
  matrix[3][1] = transform.translation.y;
  matrix[3][2] = transform.translation.z;
#endif
  return matrix;
}

void Node::UpdateModelMatrixTranslation() {
  matrix = glm::translate(mat4f{1.0}, transform.translation * vec3f(1, 1, 1));
  matrix = matrix * rot_matrix;
}

void Node::SetRotation(const vec3f& in) {
  transform.rotation = in;
  ClampRotation(transform.rotation.y);
  ClampRotation(transform.rotation.x);
  ClampRotation(transform.rotation.z);
  localMatrix();
}

void Node::SetLocation(const vec3f& in) {
  transform.translation = in;
  UpdateModelMatrixTranslation();
}

void Node::SetRotationMatrix(const mat4f& in_matrix) {
  matrix = glm::translate(mat4f{1.0}, transform.translation * vec3f(1, 1, 1));

  glm::quat quat(in_matrix);
  transform.rotation.x = glm::degrees(glm::pitch(quat));
  transform.rotation.y = glm::degrees(glm::yaw(quat));
  transform.rotation.z = glm::degrees(glm::roll(quat));

  matrix = matrix * in_matrix;
  rot_matrix = in_matrix;
}

std::string Node::GetLocalMatrixString() { return glm::to_string(matrix); }

}  // namespace lvk