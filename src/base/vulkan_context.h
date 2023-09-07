#pragma once

#include <array>
#include <string>
#include <vector>

#include "vulkan/vulkan.h"


namespace lvk {
class VulkanContext {
 public:
  VulkanContext();
  ~VulkanContext();

  const VkPipelineVertexInputStateCreateInfo& BuildVertexInputState();

  VkInstance instance() const { return instance_; }

 private:
  VkInstance instance_{VK_NULL_HANDLE};
  std::vector<std::string> supportedInstanceExtensions;
  /** @brief Set of device extensions to be enabled for this example (must be
   * set in the derived constructor) */
  std::vector<const char *> enabledDeviceExtensions;
  std::vector<const char *> enabledInstanceExtensions;

  struct _VertexInputState {
    std::array<VkVertexInputBindingDescription, 1> bindingDescriptions;
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;
    VkPipelineVertexInputStateCreateInfo inputState;
  } vertexInputState_;

  VkResult CreateInstance(bool enableValidation);
};
}  // namespace lvk
