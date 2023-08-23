#include <string>
#include <vector>

#include "vulkan/vulkan.h"

namespace lvk {

class VulkanDevice {
 public:
  VulkanDevice(VkPhysicalDevice physicalDevice);
  ~VulkanDevice();

  uint32_t GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties,
                         VkBool32 *memTypeFound = nullptr) const;
  uint32_t GetQueueFamilyIndex(VkQueueFlags queueFlags) const;
  VkResult CreateLogicalDevice(
      VkPhysicalDeviceFeatures enabledFeatures,
      const std::vector<const char *> &enabledExtensions, void *pNextChain,
      bool useSwapChain = true,
      VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT |
                                         VK_QUEUE_COMPUTE_BIT);
  VkResult CreateBuffer(VkBufferUsageFlags usageFlags,
                        VkMemoryPropertyFlags memoryPropertyFlags,
                        VkDeviceSize size, VkBuffer *buffer,
                        VkDeviceMemory *memory, void *data = nullptr);
  VkResult CreateBuffer(VkBufferUsageFlags usageFlags,
                        VkMemoryPropertyFlags memoryPropertyFlags,
                        vks::Buffer *buffer, VkDeviceSize size,
                        void *data = nullptr);
  void CopyBuffer(vks::Buffer *src, vks::Buffer *dst, VkQueue queue,
                  VkBufferCopy *copyRegion = nullptr);

  VkCommandPool CreateCommandPool(
      uint32_t queueFamilyIndex,
      VkCommandPoolCreateFlags createFlags =
          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level,
                                      VkCommandPool pool, bool begin = false);
  VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level,
                                      bool begin = false);
  void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue,
                          VkCommandPool pool, bool free = true);
  void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue,
                          bool free = true);
  bool ExtensionSupported(std::string extension);
  VkFormat GetSupportedDepthFormat(bool checkSamplingSupport);

 private:
  VkPhysicalDevice vkPhysicalDevice_;
  VkDevice logicalDevice_;
  VkPhysicalDeviceProperties properties_;
  VkPhysicalDeviceFeatures features_;
  VkPhysicalDeviceFeatures enabledFeatures_;
  VkPhysicalDeviceMemoryProperties memoryProperties_;
  std::vector<VkQueueFamilyProperties> queueFamilyProperties_;
  std::vector<std::string> supportedExtensions_;
  VkCommandPool commandPool_{VK_NULL_HANDLE};
};

}  // namespace lvk