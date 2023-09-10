#include "vulkan_buffer.h"

#include <assert.h>
#include <memory.h>

namespace lvk {

VkResult VulkanBuffer::Map(VkDeviceSize size, VkDeviceSize offset) {
  return vkMapMemory(device_, memory_, offset, size, 0, &mapped_);
}

void VulkanBuffer::Unmap() {
  if (mapped_) {
    vkUnmapMemory(device_, memory_);
    mapped_ = nullptr;
  }
}

VkResult VulkanBuffer::Bind(VkDeviceSize offset) {
  return vkBindBufferMemory(device_, buffer_, memory_, offset);
}

void VulkanBuffer::SetupDescriptor(VkDeviceSize size, VkDeviceSize offset) {
  descriptor_.offset = offset;
  descriptor_.buffer = buffer_;
  descriptor_.range = size;
}

void VulkanBuffer::CopyTo(const void* data, VkDeviceSize size) {
  assert(mapped_);
  memcpy(mapped_, data, size);
}

VkResult VulkanBuffer::Flush(VkDeviceSize size, VkDeviceSize offset) {
  VkMappedMemoryRange mappedRange = {};
  mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  mappedRange.memory = memory_;
  mappedRange.offset = offset;
  mappedRange.size = size;
  return vkFlushMappedMemoryRanges(device_, 1, &mappedRange);
}

VkResult VulkanBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset) {
  VkMappedMemoryRange mappedRange = {};
  mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  mappedRange.memory = memory_;
  mappedRange.offset = offset;
  mappedRange.size = size;
  return vkInvalidateMappedMemoryRanges(device_, 1, &mappedRange);
}

void VulkanBuffer::Update(const uint8_t *data, const VkDeviceSize size, const VkDeviceSize offset) {
  Map(size, offset);
  CopyTo(data, size);
  Unmap();
}

void VulkanBuffer::Destroy() {
  if (buffer_) {
    vkDestroyBuffer(device_, buffer_, nullptr);
  }
  if (memory_) {
    vkFreeMemory(device_, memory_, nullptr);
  }
}

}  // namespace lvk