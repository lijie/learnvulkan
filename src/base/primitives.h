#pragma once

#include <vector>

#include "vertex_data.h"
#include "vulkan_device.h"

#include "transform.h"

namespace lvk {

struct MeshSection {
  std::vector<VertexLayout> vertices;
  std::vector<uint32_t> indices;
};

struct PrimitiveMesh {
  PrimitiveMesh() {
    // one section at least
    sections.resize(1);
  }

  PrimitiveMesh(size_t n) {
    // one section at least
    sections.resize(n);
  }

  PrimitiveMesh(const MeshSection& section) { sections.push_back(section); }
  std::vector<MeshSection> sections;
};

// todo: move to vulkan context
// TODO: move to vulkan_primitives
struct PrimitiveMeshVK {
  PrimitiveMeshVK() {
    vertexBuffer = nullptr;
    indexBuffer = nullptr;
    indexCount = 0;
  }
  VulkanBuffer* vertexBuffer{nullptr};
  VulkanBuffer* indexBuffer{nullptr};
  VkPipelineVertexInputStateCreateInfo inputState;
  uint32_t indexCount{0};

  void CreateBuffer(const MeshSection* mesh, VulkanDevice* device);

  ~PrimitiveMeshVK();
};

namespace primitive_helpers {

}

namespace primitive {
PrimitiveMesh quad();
PrimitiveMesh cube(float width = 1, float height = 1, float depth = 1);
}
}  // namespace lvk
