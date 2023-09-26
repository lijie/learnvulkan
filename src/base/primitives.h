#pragma once

#include <vector>

#include "vertex_data.h"
#include "vulkan_device.h"

#include "transform.h"

namespace lvk {

struct PrimitiveMesh {
  std::vector<VertexLayout> vertices;
  std::vector<uint32_t> indices;
};

// todo: move to vulkan context
// TODO: move to vulkan_primitives
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

namespace primitive_helpers {

}

namespace primitive {
PrimitiveMesh quad();
PrimitiveMesh cube(float width = 1, float height = 1, float depth = 1);
}
}  // namespace lvk
