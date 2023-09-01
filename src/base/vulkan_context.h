#pragma once

#include "vulkan/vulkan.h"

#include <vector>
#include <string>

namespace lvk {
class VulkanContext {
 private:
  VkInstance instance{VK_NULL_HANDLE};
  std::vector<std::string> supportedInstanceExtensions;
  /** @brief Set of device extensions to be enabled for this example (must be
   * set in the derived constructor) */
  std::vector<const char *> enabledDeviceExtensions;
  std::vector<const char *> enabledInstanceExtensions;

  VkResult CreateInstance(bool enableValidation);
};
}  // namespace lvk
