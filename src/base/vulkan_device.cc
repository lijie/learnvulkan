#include "vulkan_device.h"
#include <assert.h>

namespace lvk {
VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice) {
  assert(physicalDevice);
  this->vkPhysicalDevice_ = physicalDevice;

  // Store Properties features, limits and properties of the physical device for
  // later use Device properties also contain limits and sparse properties
  vkGetPhysicalDeviceProperties(physicalDevice, &properties_);
  // Features should be checked by the examples before using them
  vkGetPhysicalDeviceFeatures(physicalDevice, &features_);
  // Memory properties are used regularly for creating all kinds of buffers
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties_);
  // Queue family properties, used for setting up requested queues upon device
  // creation
  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);
  assert(queueFamilyCount > 0);
  queueFamilyProperties_.resize(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilyProperties_.data());

  // Get list of supported extensions
  uint32_t extCount = 0;
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount,
                                       nullptr);
  if (extCount > 0) {
    std::vector<VkExtensionProperties> extensions(extCount);
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount,
                                             &extensions.front()) ==
        VK_SUCCESS) {
      for (auto ext : extensions) {
        supportedExtensions_.push_back(ext.extensionName);
      }
    }
  }
}
}  // namespace lvk