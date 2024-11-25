#pragma once

#include "vulkan/vulkan.h"

namespace lvk {
class VulkanBuffer {
  friend class VulkanDevice;

 public:
  void set_deivce(VkDevice device) { device_ = device; }
  VkBuffer buffer() { return buffer_; }
  const VkBuffer * bufferp() { return &buffer_; }
  VkDeviceMemory memory() { return memory_; }
  VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  void Unmap();
  VkResult Bind(VkDeviceSize offset = 0);
  void SetupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  void CopyTo(const void* data, VkDeviceSize size);
  VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  VkResult Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

  void Update(const uint8_t* data, const VkDeviceSize size, const VkDeviceSize offset = 0);
  void Destroy();

  void* mapped() { return mapped_; }

  static VulkanBuffer* Create(VulkanDevice *device, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags,
                              VkDeviceSize size, const void* data = nullptr);

  VkDescriptorBufferInfo descriptor_;

 private:
  VkBuffer buffer_{VK_NULL_HANDLE};
  VkDevice device_;
  VkDeviceMemory memory_{VK_NULL_HANDLE};
  VkDeviceSize size_{0};
  VkDeviceSize alignment_{0};
  void* mapped_{nullptr};
  VkBufferUsageFlags usageFlags_;
  VkMemoryPropertyFlags memoryPropertyFlags_;
};
}  // namespace lvk