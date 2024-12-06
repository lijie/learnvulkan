#include "base/node.h"

#include "glm/ext/matrix_transform.hpp"
#include "glm/trigonometric.hpp"
#include "lvk_math.h"

namespace lvk {

Node::Node() {}

mat4f Node::localMatrix() {
#if 0
  matrix = glm::translate(mat4f{1.0}, transform.translation * vec3f(1, 1, 1));
  matrix = glm::rotate(matrix, glm::radians(transform.rotation.x), vec3f(1, 0, 0));
  matrix = glm::rotate(matrix, glm::radians(transform.rotation.y), vec3f(0, 1, 0));
  matrix = glm::rotate(matrix, glm::radians(transform.rotation.z), vec3f(0, 0, 1));
#else

  // 这里不考虑效率, 让计算过程清晰易懂.

  matrix = glm::identity<mat4f>();

  matrix[3][0] = transform.translation.x;
  matrix[3][1] = transform.translation.y;
  matrix[3][2] = transform.translation.z;

  float SX = sin(glm::radians(transform.rotation.x));
  float CX = cos(glm::radians(transform.rotation.x));

  mat4f RMX = {
      {1.0f, 0.0f, 0.0f, 0.0f},
      {0.0f, CX, SX, 0.0f},
      {0.0f, SX, CX, 0.0f},
      {0.0f, 0.0f, 0.0f, 1.0f},
  };

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
#endif
  return matrix;
}

std::string Node::GetLocalMatrixString() { return glm::to_string(matrix); }

}  // namespace lvk