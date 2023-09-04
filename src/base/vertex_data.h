#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#include <vector>

#include "glm/glm.hpp"

namespace lvk {

using Vector = glm::vec3;
using Vector3 = Vector;
using Vector2 = glm::vec2;

struct VertexLayout {
  Vector3 position;
  Vector3 normal;
  Vector2 uv;
};

class VulkanBuffer;

class StaticMesh {
 public:
  StaticMesh(std::vector<VertexLayout>&& vertices, std::vector<uint32_t>&& indices_) {
    vertices_ = std::move(vertices);
    indices_ = std::move(indices_);
  }

  uint32_t VertexSize() const {
    return static_cast<uint32_t>(sizeof(VertexLayout) * vertices_.size());
  }

  const void* VertexData() const {
    return vertices_.data();
  }

  uint32_t IndexSize() const {
    return static_cast<uint32_t>(sizeof(uint32_t) *indices_.size());
  }

  const void* IndexData() const {
    return indices_.data();
  }

  uint32_t IndexCount() const {
    return static_cast<uint32_t>(indices_.size());
  }

  StaticMesh() {}

 protected:
  std::vector<VertexLayout> vertices_;
  std::vector<uint32_t> indices_;

  VulkanBuffer* vertexBuffer_{nullptr};
  VulkanBuffer* indexBuffer_{nullptr};
};

class QuadMesh : public StaticMesh {
 public:
  QuadMesh() {
    // Setup vertices for a single uv-mapped quad made from two triangles
    vertices_ = {{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                 {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                 {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                 {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}};

    // Setup indices
    indices_ = {0, 1, 2, 2, 3, 0};

    StaticMesh::StaticMesh();
  }

  static const StaticMesh *instance() { return &instance_; }

private:
  static QuadMesh instance_;
};

class VulkanContext;
class RenderObject {
 public:
  RenderObject(VulkanContext* ctx, StaticMesh* mesh);
};

struct VertexInputState {
  std::vector<VkVertexInputAttributeDescription> attribtues;
  uint32_t stride;
};

}  // namespace lvk