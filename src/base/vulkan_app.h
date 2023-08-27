#pragma once

#include <string>
#include <vector>

#include "commandline_parser.h"
#include "vulkan/vulkan.h"
#include "vulkan_swapchain.h"

namespace lvk {
class VulkanDevice;
class VulkanApp {
 protected:
  VkInstance instance;

  struct Settings {
    /** @brief Activates validation layers (and message output) when set to true
     */
    bool validation = false;
    /** @brief Set to true if fullscreen mode has been requested via command
     * line */
    bool fullscreen = false;
    /** @brief Set to true if v-sync will be forced for the swapchain */
    bool vsync = false;
    /** @brief Enable UI overlay */
    bool overlay = true;
  } settings;

  std::string title = "Vulkan Example";
  std::string name = "vulkanExample";
  uint32_t apiVersion = VK_API_VERSION_1_0;

  std::vector<std::string> supportedInstanceExtensions;

  /** @brief Set of device extensions to be enabled for this example (must be
   * set in the derived constructor) */
  std::vector<const char *> enabledDeviceExtensions;
  std::vector<const char *> enabledInstanceExtensions;

  VulkanDevice *vulkanDevice{nullptr};
  VkPhysicalDevice physicalDevice;
  VkPhysicalDeviceProperties deviceProperties;
  // Stores the features available on the selected physical device (for e.g.
  // checking if a feature is available)
  VkPhysicalDeviceFeatures deviceFeatures;
  // Stores all available memory (type) properties for the physical device
  VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
  VkPhysicalDeviceFeatures enabledFeatures{};
  void *deviceCreatepNextChain = nullptr;
  VkDevice device;
  VkQueue queue;
  // Depth buffer format (selected during Vulkan initialization)
  VkFormat depthFormat;
  VulkanSwapchain swapChain;
  // Synchronization semaphores
  struct {
    // Swap chain image presentation
    VkSemaphore presentComplete;
    // Command buffer submission and execution
    VkSemaphore renderComplete;
  } semaphores;
  VkSubmitInfo submitInfo;
  /** @brief Pipeline stages used to wait at for graphics queue submissions */
  VkPipelineStageFlags submitPipelineStages =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  bool requiresStencil{false};
  CommandLineParser commandLineParser;

  VkResult CreateInstance(bool enableValidation);
  bool InitVulkan();
  virtual void GetEnabledFeatures(){};
  virtual void GetEnabledExtensions() {}
};
}  // namespace lvk