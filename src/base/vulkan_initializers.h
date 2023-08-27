#include <vulkan/vulkan.h>

namespace lvk {
namespace initializers {

inline VkMemoryAllocateInfo MemoryAllocateInfo() {
  VkMemoryAllocateInfo memAllocInfo{};
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  return memAllocInfo;
}

inline VkMappedMemoryRange MappedMemoryRange() {
  VkMappedMemoryRange mappedMemoryRange{};
  mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  return mappedMemoryRange;
}

inline VkCommandBufferAllocateInfo CommandBufferAllocateInfo(
    VkCommandPool commandPool, VkCommandBufferLevel level,
    uint32_t bufferCount) {
  VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
  commandBufferAllocateInfo.sType =
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocateInfo.commandPool = commandPool;
  commandBufferAllocateInfo.level = level;
  commandBufferAllocateInfo.commandBufferCount = bufferCount;
  return commandBufferAllocateInfo;
}

inline VkCommandBufferBeginInfo CommandBufferBeginInfo() {
  VkCommandBufferBeginInfo cmdBufferBeginInfo{};
  cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  return cmdBufferBeginInfo;
}

inline VkBufferCreateInfo BufferCreateInfo(VkBufferUsageFlags usage,
                                           VkDeviceSize size) {
  VkBufferCreateInfo bufCreateInfo{};
  bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufCreateInfo.usage = usage;
  bufCreateInfo.size = size;
  return bufCreateInfo;
}

inline VkSubmitInfo SubmitInfo() {
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  return submitInfo;
}

inline VkSemaphoreCreateInfo SemaphoreCreateInfo() {
  VkSemaphoreCreateInfo semaphoreCreateInfo{};
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  return semaphoreCreateInfo;
}

inline VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0) {
  VkFenceCreateInfo fenceCreateInfo{};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.flags = flags;
  return fenceCreateInfo;
}
}  // namespace initializers
}  // namespace lvk