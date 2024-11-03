#include <iostream>
#include <string>

#include <assert.h>

#include "vulkan/vulkan.h"

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

#define VK_CHECK_RESULT(f)                                                  \
  {                                                                         \
    VkResult res = (f);                                                     \
    if (res != VK_SUCCESS) {                                                \
      std::cout << "Fatal : VkResult is \"" << lvk::tools::ErrorString(res) \
                << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
      assert(res == VK_SUCCESS);                                            \
    }                                                                       \
  }

namespace lvk {
namespace tools {
std::string ErrorString(VkResult result);

std::string GetAssetPath();
std::string GetModelPath();

// Display error message and exit on fatal error
void ExitFatal(const std::string& message, int32_t exitCode);
void ExitFatal(const std::string& message, VkResult resultCode);

std::string PhysicalDeviceTypeString(VkPhysicalDeviceType type);

// Selected a suitable supported depth format starting with 32 bit down to 16
// bit Returns false if none of the depth formats in the list is supported by
// the device
VkBool32 GetSupportedDepthFormat(VkPhysicalDevice physicalDevice,
                                 VkFormat* depthFormat);
// Same as getSupportedDepthFormat but will only select formats that also have
// stencil
VkBool32 GetSupportedDepthStencilFormat(VkPhysicalDevice physicalDevice,
                                        VkFormat* depthStencilFormat);

VkShaderModule LoadShader(const char* fileName, VkDevice device);
}  // namespace tools
}  // namespace lvk