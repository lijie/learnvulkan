#pragma once

#include <chrono>
#include <string>
#include <vector>
#include "window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "commandline_parser.h"
// #include "vulkan/vulkan.h"
#include "vulkan_swapchain.h"

namespace lvk {
class VulkanDevice;
class VulkanContext;
class VulkanApp {
 protected:
  // VkInstance instance;
  VulkanContext *context_{nullptr};

  struct Settings {
    /** @brief Activates validation layers (and message output) when set to true
     */
    bool validation = true;
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
  // VkQueue queue;
  // Depth buffer format (selected during Vulkan initialization)
  VkFormat depthFormat;
  // VulkanSwapchain swapChain;
  // VkCommandPool cmdPool;
  // Synchronization semaphores
  #if 0
  struct {
    // Swap chain image presentation
    VkSemaphore presentComplete;
    // Command buffer submission and execution
    VkSemaphore renderComplete;
  } semaphores;
  #endif
  // VkSubmitInfo submitInfo;
  // std::vector<VkCommandBuffer> drawCmdBuffers;
  /** @brief Pipeline stages used to wait at for graphics queue submissions */
  // VkPipelineStageFlags submitPipelineStages =
  //    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  // std::vector<VkFence> waitFences;
  // struct {
  //   VkImage image;
  //   VkDeviceMemory mem;
  //   VkImageView view;
  // } depthStencil;
  // VkRenderPass renderPass = VK_NULL_HANDLE;
  // VkPipelineCache pipelineCache;
  // std::vector<VkFramebuffer> frameBuffers;
  // Active frame buffer index
  // uint32_t currentBuffer = 0;
  // VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  bool prepared{false};

  //std::vector<VkShaderModule> shaderModules;

  // win32 window
  // HWND window;
  // void *window{nullptr};
  Window *window_;
  HINSTANCE windowInstance;
  uint32_t width = 1280;
  uint32_t height = 720;
  uint32_t destWidth = 0;
  uint32_t destHeight = 0;

  bool requiresStencil{false};

  std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp,
      tPrevEnd;
  uint32_t frameCounter = 0;

  CommandLineParser commandLineParser;

  void InitSwapchain();
  void SetupSwapchain();
  void CreateCommandPool();
  void CreateCommandBuffers();
  void CreateSynchronizationPrimitives();
  // void SetupDepthStencil();
  // void SetupRenderPass();
  // void CreatePipelineCache();
  // void SetupFrameBuffer();
  virtual void Render(double deltaTime = 0) = 0;
  void NextFrame(double deltaTime);
  void PrepareFrame();
  void SubmitFrame();

  std::string GetWindowTitle();

  virtual void GetEnabledFeatures(){};
  virtual void GetEnabledExtensions() {}

  std::string GetShadersPath() const;
  VkPipelineShaderStageCreateInfo LoadShader(std::string fileName,
                                             VkShaderStageFlagBits stage);

 public:
  static std::vector<const char *> args;

  virtual void Prepare();
  void RenderLoop();
  bool InitVulkan();
  // window
  void HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  HWND SetupWindow(HINSTANCE hinstance, WNDPROC wndproc);

  virtual ~VulkanApp() {}
};
}  // namespace lvk