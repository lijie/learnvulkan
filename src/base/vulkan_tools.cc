#include "vulkan_tools.h"

#include <iostream>
#include <vector>
namespace lvk {
namespace tools {

bool errorModeSilent = false;

std::string ErrorString(VkResult result) {
  switch (result) {
#define STR(r) \
  case VK_##r: \
    return #r
    STR(NOT_READY);
    STR(TIMEOUT);
    STR(EVENT_SET);
    STR(EVENT_RESET);
    STR(INCOMPLETE);
    STR(ERROR_OUT_OF_HOST_MEMORY);
    STR(ERROR_OUT_OF_DEVICE_MEMORY);
    STR(ERROR_INITIALIZATION_FAILED);
    STR(ERROR_DEVICE_LOST);
    STR(ERROR_MEMORY_MAP_FAILED);
    STR(ERROR_LAYER_NOT_PRESENT);
    STR(ERROR_EXTENSION_NOT_PRESENT);
    STR(ERROR_FEATURE_NOT_PRESENT);
    STR(ERROR_INCOMPATIBLE_DRIVER);
    STR(ERROR_TOO_MANY_OBJECTS);
    STR(ERROR_FORMAT_NOT_SUPPORTED);
    STR(ERROR_SURFACE_LOST_KHR);
    STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
    STR(SUBOPTIMAL_KHR);
    STR(ERROR_OUT_OF_DATE_KHR);
    STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
    STR(ERROR_VALIDATION_FAILED_EXT);
    STR(ERROR_INVALID_SHADER_NV);
    STR(ERROR_INCOMPATIBLE_SHADER_BINARY_EXT);
#undef STR
    default:
      return "UNKNOWN_ERROR";
  }
}

void ExitFatal(const std::string& message, int32_t exitCode) {
#if defined(_WIN32)
  if (!errorModeSilent) {
    MessageBox(NULL, message.c_str(), NULL, MB_OK | MB_ICONERROR);
  }
#elif defined(__ANDROID__)
  LOGE("Fatal error: %s", message.c_str());
  vks::android::showAlert(message.c_str());
#endif
  std::cerr << message << "\n";
#if !defined(__ANDROID__)
  exit(exitCode);
#endif
}

void ExitFatal(const std::string& message, VkResult resultCode) {
  ExitFatal(message, (int32_t)resultCode);
}

std::string PhysicalDeviceTypeString(VkPhysicalDeviceType type) {
  switch (type) {
#define STR(r)                      \
  case VK_PHYSICAL_DEVICE_TYPE_##r: \
    return #r
    STR(OTHER);
    STR(INTEGRATED_GPU);
    STR(DISCRETE_GPU);
    STR(VIRTUAL_GPU);
    STR(CPU);
#undef STR
    default:
      return "UNKNOWN_DEVICE_TYPE";
  }
}

VkBool32 GetSupportedDepthFormat(VkPhysicalDevice physicalDevice,
                                 VkFormat* depthFormat) {
  // Since all depth formats may be optional, we need to find a suitable depth
  // format to use Start with the highest precision packed format
  std::vector<VkFormat> formatList = {
      VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM};

  for (auto& format : formatList) {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
    if (formatProps.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      *depthFormat = format;
      return true;
    }
  }

  return false;
}

VkBool32 GetSupportedDepthStencilFormat(VkPhysicalDevice physicalDevice,
                                        VkFormat* depthStencilFormat) {
  std::vector<VkFormat> formatList = {
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM_S8_UINT,
  };

  for (auto& format : formatList) {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
    if (formatProps.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      *depthStencilFormat = format;
      return true;
    }
  }

  return false;
}
}  // namespace tools
}  // namespace lvk