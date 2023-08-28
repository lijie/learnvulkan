#pragma once

#include <vector>

#include "vulkan/vulkan.h"

namespace lvk {

typedef struct _SwapchainBuffers {
  VkImage image;
  VkImageView view;
} SwapchainBuffer;

class VulkanSwapchain {
  friend class VulkanApp;

 private:
  VkInstance instance_;
  VkDevice device_;
  VkPhysicalDevice physicalDevice_;
  VkSurfaceKHR surface_;
  VkFormat colorFormat_;
  VkColorSpaceKHR colorSpace_;
  VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
  uint32_t imageCount_;
  std::vector<VkImage> images_;
  std::vector<SwapchainBuffer> buffers_;
  uint32_t queueNodeIndex_ = UINT32_MAX;

 public:
  void InitSurface(void* platformHandle, void* platformWindow);
  void Connect(VkInstance instance, VkPhysicalDevice physicalDevice,
               VkDevice device);
  void Create(uint32_t* width, uint32_t* height, bool vsync = false,
              bool fullscreen = false);
  VkResult AcquireNextImage(VkSemaphore presentCompleteSemaphore,
                            uint32_t* imageIndex);
  VkResult QueuePresent(VkQueue queue, uint32_t imageIndex,
                        VkSemaphore waitSemaphore = VK_NULL_HANDLE);
  void Cleanup();

  uint32_t queueNodeIndex() { return queueNodeIndex_; }
  uint32_t imageCount() { return imageCount_; }
};
}  // namespace lvk