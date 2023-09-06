#include "vertex_data.h"

#include "vulkan_buffer.h"
#include "vulkan_device.h"
#include "vulkan_tools.h"
#include "vulkan_initializers.h"

namespace lvk {

#define VERTEX_BUFFER_BIND_ID 0

void VertexRawData::SetupForRendering(VulkanDevice* device) {
    CreateBuffers(device);
    SetupVertexDescriptions();
}
void VertexRawData::SetupVertexDescriptions() {
  // Binding description
  bindingDescriptions_[0] = lvk::initializers::VertexInputBindingDescription(
      VERTEX_BUFFER_BIND_ID, sizeof(VertexLayout), VK_VERTEX_INPUT_RATE_VERTEX);

  // Attribute descriptions
  // Describes memory layout and shader positions
  // Location 0 : Position
  attributeDescriptions_[0] = lvk::initializers::VertexInputAttributeDescription(
      VERTEX_BUFFER_BIND_ID, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexLayout, position));
  // Location 1 : Vertex normal
  attributeDescriptions_[2] = lvk::initializers::VertexInputAttributeDescription(
      VERTEX_BUFFER_BIND_ID, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexLayout, normal));
  // Location 2 : Texture coordinates
  attributeDescriptions_[1] = lvk::initializers::VertexInputAttributeDescription(
      VERTEX_BUFFER_BIND_ID, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexLayout, uv));

  inputState_ = lvk::initializers::PipelineVertexInputStateCreateInfo();
  inputState_.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions_.size());
  inputState_.pVertexBindingDescriptions = bindingDescriptions_.data();
  inputState_.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions_.size());
  inputState_.pVertexAttributeDescriptions = attributeDescriptions_.data();
}

bool VertexRawData::CreateBuffers(VulkanDevice* device) {
  if (vertexBuffer_ && indexBuffer_) return true;
  vertexBuffer_ = new VulkanBuffer();
  indexBuffer_ = new VulkanBuffer();
  // Create buffers
  // For the sake of simplicity we won't stage the vertex data to the gpu memory
  // Vertex buffer
  VK_CHECK_RESULT(device->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       vertexBuffer_, VertexSize(), VertexData()));
  // &vertexBuffer, vertices.size() * sizeof(Vertex), vertices.data()));
  // Index buffer
  VK_CHECK_RESULT(device->CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       indexBuffer_, IndexSize(), IndexData()));
  // &indexBuffer, indices.size() * sizeof(uint32_t), indices.data()));
  return true;
}

StaticMesh* QuadMeshInstance{nullptr};

StaticMesh* QuadMesh::instance() {
  if (QuadMeshInstance == nullptr) {
    QuadMeshInstance = new StaticMesh(new QuadVertexRawData());
  }
  return QuadMeshInstance;
}

}  // namespace lvk