#pragma once

#include <vector>

#define GLM_FORCE_RADIANS
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

// todo: move to vulkan context
struct PrimitiveMeshVK {
  VulkanBuffer* vertexBuffer{nullptr};
  VulkanBuffer* indexBuffer{nullptr};
  std::array<VkVertexInputBindingDescription, 1> bindingDescriptions;
  std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;
  VkPipelineVertexInputStateCreateInfo inputState;
  uint32_t indexCount{0};

  void CreateBuffer(const PrimitiveMesh* mesh, VulkanDevice* device);

  ~PrimitiveMeshVK();
};

struct Transform {
  vec3f translation;
  vec3f rotation;
  vec3f scale;  
};

namespace primitive_helpers {

}

namespace primitive {
PrimitiveMesh quad();
PrimitiveMesh cube(float width = 1, float height = 1, float depth = 1);
}
}  // namespace lvk
