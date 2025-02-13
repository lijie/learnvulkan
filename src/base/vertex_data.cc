#include "vertex_data.h"

#include "vulkan_buffer.h"
#include "vulkan_device.h"
#include "vulkan_initializers.h"
#include "vulkan_tools.h"

#define VERTEX_BUFFER_BIND_ID 0

namespace lvk {

// TODO: support non-interleaved vertex data
VkPipelineVertexInputStateCreateInfo VertexLayout::GetPiplineVertexInputState() {
  static std::vector<VkVertexInputBindingDescription> bindingDescriptions = {
      lvk::initializers::VertexInputBindingDescription(VERTEX_BUFFER_BIND_ID, sizeof(VertexLayout),
                                                       VK_VERTEX_INPUT_RATE_VERTEX)};

  static std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
  // Attribute descriptions
  // Describes memory layout and shader positions
  // Location 0 : Position
  attributeDescriptions.push_back(lvk::initializers::VertexInputAttributeDescription(
      VERTEX_BUFFER_BIND_ID, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexLayout, position)));
  // Location 1 : Vertex normal
  attributeDescriptions.push_back(lvk::initializers::VertexInputAttributeDescription(
      VERTEX_BUFFER_BIND_ID, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexLayout, normal)));
  // Location 2 : Texture coordinates
  attributeDescriptions.push_back(lvk::initializers::VertexInputAttributeDescription(
      VERTEX_BUFFER_BIND_ID, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexLayout, uv)));

  VkPipelineVertexInputStateCreateInfo inputState = lvk::initializers::PipelineVertexInputStateCreateInfo();
  inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
  inputState.pVertexBindingDescriptions = bindingDescriptions.data();
  inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  inputState.pVertexAttributeDescriptions = attributeDescriptions.data();
  return inputState;
}

}  // namespace lvk