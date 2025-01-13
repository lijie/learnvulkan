#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "input.h"
#include "lvk_math.h"
#include "window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "commandline_parser.h"
// #include "vulkan/vulkan.h"
#include "scene.h"
#include "vulkan_swapchain.h"
#include "vulkan_ui.h"

#define ENABLE_RENDERDOC 1

namespace lvk {
class VulkanDevice;
class VulkanContext;
class Camera;

class DefaultCameraMoveInput : public InputComponent {
 public:
  DefaultCameraMoveInput(Camera *camera) : camera_(camera), input_vec_(ZERO_VECTOR), rotation_vec_(ZERO_VECTOR) {
    mouse_move_delta_ = vec2f(0, 0);
  }
  virtual void OnDirectionInput(const DirectionInput &di) override;
  virtual void OnRotationInput(const DirectionInput &ri) override;
  virtual void OnMouseClick(MouseButton button, MouseState action) override;
  virtual void OnMouseMove(double x, double y) override;
  virtual void OnKey(KeyCode key, KeyState action) override;
  void Update(float delta_time);

  void UpdateMouseLookMode(float delta_time);

 private:
  Camera *camera_;
  vec3f input_vec_;
  vec3f rotation_vec_;
  float move_speed_ = 10;
  float rotation_speed_ = 5.0;

  // mouse
  bool mouse_left_down_ = false;
  bool init_position_ = false;
  vec2f last_mouse_position_;
  vec2f mouse_move_delta_;

  // key
  KeyState key_state_[4];

  // look mode
  int mouse_move_dir_look_mode_{0};
};

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
  bool prepared{false};

  // std::vector<VkShaderModule> shaderModules;

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

  std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp, tPrevEnd;
  uint32_t frameCounter = 0;

  CommandLineParser commandLineParser;

  // TODO: move to subsystem manager ?
  InputSystem input_system_;

  Scene scene;
  VulkanUIRenderWrapper *ui_render_wrapper_{nullptr};
  DefaultCameraMoveInput *camera_move_input_{nullptr};

  // enable renderdoc intergration
  bool enable_renderdoc{false};

  void InitSwapchain();
  void SetupSwapchain();
  void CreateCommandPool();
  void CreateCommandBuffers();
  void CreateSynchronizationPrimitives();
  // void SetupDepthStencil();
  // void SetupRenderPass();
  // void CreatePipelineCache();
  // void SetupFrameBuffer();
  virtual void Render(double deltaTime = 0);
  void NextFrame(double deltaTime);
  void PrepareFrame();
  void SubmitFrame();

  std::string GetWindowTitle();

  virtual void GetEnabledFeatures(){};
  virtual void GetEnabledExtensions() {}

  std::string GetShadersPath() const;

 protected:
  virtual void InitScene() {}
  void UpdateOverlay(Scene* scene);

  private:
    VulkanUI ui_;
#ifdef ENABLE_RENDERDOC
    void *rdoc_api_{nullptr};
#endif

 public:
  static std::vector<const char *> args;
  
  virtual void Prepare();
  void RenderLoop();
  bool InitVulkan();
  // window
  void HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  HWND SetupWindow(HINSTANCE hinstance, WNDPROC wndproc);
  virtual void Update(float delta_time);

  virtual void SetupUI(VulkanUI *ui) {};

  void StartRenderdoc();
  void EndRenderdoc();

  virtual ~VulkanApp() {}
};
}  // namespace lvk