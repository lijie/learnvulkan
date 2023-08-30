#pragma once

#include "vulkan/vulkan.h"

namespace lvk {
class VulkanBuffer {
  friend class VulkanDevice;

 public:
  void set_deivce(VkDevice device) { device_ = device; }
  VkBuffer buffer() { return buffer_; }
  VkDeviceMemory memory() { return memory_; }
  VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  void Unmap();
  VkResult Bind(VkDeviceSize offset = 0);
  void SetupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE,
                       VkDeviceSize offset = 0);
  void CopyTo(void* data, VkDeviceSize size);
  VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE,
                      VkDeviceSize offset = 0);
  void Destroy();

  void* mapped() { return mapped_; }

  VkDescriptorBufferInfo descriptor_;
  VkBuffer buffer_{VK_NULL_HANDLE};

 private:
  VkDevice device_;
  VkDeviceMemory memory_{VK_NULL_HANDLE};
  VkDeviceSize size_{0};
  VkDeviceSize alignment_{0};
  void* mapped_{nullptr};
  VkBufferUsageFlags usageFlags_;
  VkMemoryPropertyFlags memoryPropertyFlags_;
};
}  // namespace lvk