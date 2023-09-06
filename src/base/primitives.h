#pragma once

#include <vector>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "vertex_data.h"

namespace lvk {

using vec3f = glm::vec3;
using vec2f = glm::vec2;
using mat4f = glm::mat4x4;

struct PrimitiveMesh {
  std::vector<VertexLayout> vertices;
  std::vector<uint32_t> indices;
};

// render node in scene
struct Node {
  vec3f translation;
  vec3f rotation;
  vec3f scale;
  mat4f matrix;
  int material{0};
  int mesh{-1};

  mat4f localMatrix() {
    mat4f m;
    m = glm::scale(m, scale);
    m = glm::rotate(m, scale.y, vec3f(0, 1, 0));
    m = glm::rotate(m, scale.x, vec3f(1, 0, 0));
    m = glm::rotate(m, scale.z, vec3f(0, 0, 1));
    m = glm::translate(m, translation);
    return m;
  }
};

PrimitiveMesh quad();
}  // namespace lvk
