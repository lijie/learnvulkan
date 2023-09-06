#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#include <array>
#include <vector>

#include "glm/glm.hpp"
#include "vulkan_buffer.h"

namespace lvk {

using Vector = glm::vec3;
using Vector3 = Vector;
using Vector2 = glm::vec2;

struct VertexLayout {
  Vector3 position;
  Vector3 normal;
  Vector2 uv;
};

class VulkanDevice;
class VertexRawData {
  friend class StaticMesh;

 public:
  VertexRawData(std::vector<VertexLayout>&& vertices, std::vector<uint32_t>&& indices) {
    vertices_ = std::move(vertices);
    indices_ = std::move(indices);
  }
  uint32_t VertexSize() const { return static_cast<uint32_t>(sizeof(VertexLayout) * vertices_.size()); }

  const void* VertexData() const { return vertices_.data(); }

  uint32_t IndexSize() const { return static_cast<uint32_t>(sizeof(uint32_t) * indices_.size()); }

  const void* IndexData() const { return indices_.data(); }

  uint32_t IndexCount() const { return static_cast<uint32_t>(indices_.size()); }

  void SetupForRendering(VulkanDevice* device);
  bool CreateBuffers(VulkanDevice* device);

  void SetupVertexDescriptions();

  VulkanBuffer* vertexBuffer() { return vertexBuffer_; }
  VulkanBuffer* indexBuffer() { return indexBuffer_; }

 private:
  std::vector<VertexLayout> vertices_;
  std::vector<uint32_t> indices_;
  VulkanBuffer* vertexBuffer_{nullptr};
  VulkanBuffer* indexBuffer_{nullptr};
  std::array<VkVertexInputBindingDescription, 1> bindingDescriptions_;
  std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions_;
  VkPipelineVertexInputStateCreateInfo inputState_;
};

class QuadVertexRawData : public VertexRawData {
 public:
  QuadVertexRawData()
      : VertexRawData({{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                       {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                       {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                       {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}},
                      {0, 1, 2, 2, 3, 0}) {}
};

class VulkanBuffer;

class StaticMesh {
 public:
  StaticMesh(VertexRawData* rawData) : rawData_(rawData) {}

  uint32_t VertexSize() const { return rawData_->VertexSize(); }

  const void* VertexData() const { return rawData_->VertexData(); }

  uint32_t IndexSize() const { return rawData_->IndexSize(); }

  const void* IndexData() const { return rawData_->IndexData(); }

  uint32_t IndexCount() const { return rawData_->IndexCount(); }

  VulkanBuffer* vertexBuffer() { return rawData_->vertexBuffer_; }
  VulkanBuffer* indexBuffer() { return rawData_->indexBuffer_; }
  VkBuffer getVertexVkBuffer() { return vertexBuffer()->buffer(); }
  VkBuffer getIndexVkBuffer() { return indexBuffer()->buffer(); }

  const VkPipelineVertexInputStateCreateInfo& GetVkPipelineVertexInputStateCreateInfo() {
    return rawData_->inputState_;
  }

  void InitForRendering(VulkanDevice* device) { rawData_->SetupForRendering(device); }

 protected:
  VertexRawData* rawData_{nullptr};

  VulkanBuffer* vertexBuffer_{nullptr};
  VulkanBuffer* indexBuffer_{nullptr};
};

class QuadMesh : public StaticMesh {
 public:
  static StaticMesh* instance();
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