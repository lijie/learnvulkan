#pragma once

#include <vector>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "vertex_data.h"
#include "vulkan_device.h"

namespace lvk {

using vec3f = glm::vec3;
using vec2f = glm::vec2;
using mat4f = glm::mat4x4;

struct PrimitiveMesh {
  std::vector<VertexLayout> vertices;
  std::vector<uint32_t> indices;
};

struct PrimitiveMeshVK {
  VulkanBuffer* vertexBuffer{nullptr};
  VulkanBuffer* indexBuffer{nullptr};
  std::array<VkVertexInputBindingDescription, 1> bindingDescriptions;
  std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;
  VkPipelineVertexInputStateCreateInfo inputState;

  void CreateBuffer(PrimitiveMesh* mesh, VulkanDevice* device);

  ~PrimitiveMeshVK();
};

struct Transform {
  vec3f translation;
  vec3f rotation;
  vec3f scale;  
};

// render node in scene
struct Node {
  Transform transform;
  mat4f matrix;
  int material{0};
  int mesh{-1};

  mat4f localMatrix() {
    mat4f& m = matrix;
    m = glm::scale(m, transform.scale);
    m = glm::rotate(m, transform.rotation.y, vec3f(0, 1, 0));
    m = glm::rotate(m, transform.rotation.x, vec3f(1, 0, 0));
    m = glm::rotate(m, transform.rotation.z, vec3f(0, 0, 1));
    m = glm::translate(m, transform.translation);
    return m;
  }
};

namespace primitive_helpers {

}

namespace primitive {
PrimitiveMesh quad();
}
}  // namespace lvk
