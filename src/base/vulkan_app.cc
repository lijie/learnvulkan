#include "vulkan_app.h"

#include <array>
#include <iostream>
#include <ratio>
#include <vector>

#include "input.h"
#include "lvk_math.h"
#include "vulkan_context.h"
#include "vulkan_debug.h"
#include "vulkan_device.h"
#include "vulkan_initializers.h"
#include "vulkan_tools.h"
#include "window.h"

#include "lvk_log.h"

namespace lvk {
std::vector<const char *> VulkanApp::args;

void DefaultCameraMoveInput::OnDirectionInput(const DirectionInput &di) {
  DEBUG_LOG("direction input: {}, {}", (int)di.direction, di.scale);
  if (di.direction == kInputDirection::Forward) {
    input_vec_ += camera_->GetForwardVector() * move_speed_ * di.scale;
  } else if (di.direction == kInputDirection::Right) {
    input_vec_ += camera_->GetRightVector() * move_speed_ * di.scale;
  }
}

void DefaultCameraMoveInput::OnRotationInput(const DirectionInput &ri) {
  DEBUG_LOG("rotation input: {}, {}", (int)ri.direction, ri.scale);
  if (ri.direction == kInputDirection::Forward) {
    rotation_vec_ += vector::RIGHT * rotation_speed_ * ri.scale;
  } else if (ri.direction == kInputDirection::Right) {
    rotation_vec_ += vector::UP * rotation_speed_ * ri.scale;
  }
}

void DefaultCameraMoveInput::Update(float delta_time) {
  camera_->SetLocation(camera_->GetLocation() + input_vec_);
  // DEBUG_LOG("Camera Location: {}", glm::to_string(camera_->GetLocation()));
  input_vec_ = ZERO_VECTOR;
  camera_->SetRotation(camera_->GetRotation() + rotation_vec_);
  rotation_vec_ = ZERO_VECTOR;
}

bool VulkanApp::InitVulkan() {
  VkResult err;
  VulkanContextOptions ctx_options;

  context_ = new VulkanContext();
  window_ = Window::NewWindow(WindowType::Glfw, width, height);
  ctx_options.window = window_;

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
  vks::android::loadVulkanFunctions(instance);
#endif

  // If requested, we enable the default validation layers for debugging
  if (settings.validation) {
    debug::SetupDebugging(context_->instance());
  }

  // Physical device
  uint32_t gpuCount = 0;
  // Get number of available physical devices
  VK_CHECK_RESULT(vkEnumeratePhysicalDevices(context_->instance(), &gpuCount, nullptr));
  if (gpuCount == 0) {
    tools::ExitFatal("No device with Vulkan support found", -1);
    return false;
  }
  // Enumerate devices
  std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
  err = vkEnumeratePhysicalDevices(context_->instance(), &gpuCount, physicalDevices.data());
  if (err) {
    tools::ExitFatal("Could not enumerate physical devices : \n" + tools::ErrorString(err), err);
    return false;
  }

  // GPU selection

  // Select physical device to be used for the Vulkan example
  // Defaults to the first device unless specified by command line
  uint32_t selectedDevice = 0;

#if !defined(VK_USE_PLATFORM_ANDROID_KHR)
  // GPU selection via command line argument
  if (commandLineParser.isSet("gpuselection")) {
    uint32_t index = commandLineParser.getValueAsInt("gpuselection", 0);
    if (index > gpuCount - 1) {
      std::cerr << "Selected device index " << index
                << " is out of range, reverting to device 0 (use -listgpus to "
                   "show available Vulkan devices)"
                << "\n";
    } else {
      selectedDevice = index;
    }
  }
  if (1) {
    std::cout << "Available Vulkan devices"
              << "\n";
    for (uint32_t i = 0; i < gpuCount; i++) {
      VkPhysicalDeviceProperties deviceProperties;
      vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
      std::cout << "Device [" << i << "] : " << deviceProperties.deviceName << std::endl;
      std::cout << " Type: " << tools::PhysicalDeviceTypeString(deviceProperties.deviceType) << "\n";
      std::cout << " API: " << (deviceProperties.apiVersion >> 22) << "."
                << ((deviceProperties.apiVersion >> 12) & 0x3ff) << "." << (deviceProperties.apiVersion & 0xfff)
                << "\n";
    }
  }
#endif

  physicalDevice = physicalDevices[selectedDevice];
  std::cout << "select physical device: " << selectedDevice << std::endl;

  // Store properties (including limits), features and memory properties of the
  // physical device (so that examples can check against them)
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
  vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
  std::cout << "Get Device Properties " << std::endl;

  // Derived examples can override this to set actual features (based on above
  // readings) to enable for logical device creation
  GetEnabledFeatures();
  std::cout << "GetEnabledFeatures " << std::endl;

  // Vulkan device creation
  // This is handled by a separate class that gets a logical device
  // representation and encapsulates functions related to a device
  vulkanDevice = new lvk::VulkanDevice(physicalDevice);

  // Derived examples can enable extensions based on the list of supported
  // extensions read from the physical device
  GetEnabledExtensions();
  std::cout << "GetEnabledExtensions " << std::endl;

  VkResult res = vulkanDevice->CreateLogicalDevice(enabledFeatures, enabledDeviceExtensions, deviceCreatepNextChain);
  if (res != VK_SUCCESS) {
    tools::ExitFatal("Could not create Vulkan device: \n" + tools::ErrorString(res), res);
    return false;
  }
  device = vulkanDevice->logicalDevice_;

  // Get a graphics queue from the device
  // vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices_.graphics, 0, &queue);

  // Find a suitable depth and/or stencil format
  VkBool32 validFormat{false};
  // Sample that make use of stencil will require a depth + stencil format, so
  // we select from a different list
  if (requiresStencil) {
    validFormat = tools::GetSupportedDepthStencilFormat(physicalDevice, &depthFormat);
  } else {
    validFormat = tools::GetSupportedDepthFormat(physicalDevice, &depthFormat);
  }
  assert(validFormat);
  ctx_options.depthFormat = depthFormat;

  context_->set_vulkan_device(vulkanDevice);
  context_->InitWithOptions(ctx_options, physicalDevice);

  WindowEventCallback callbacks{
      .OnKey = [this](int key, int action) { input_system_.OnKey(key, action); },
  };

  window_->SetEventCallbacks(callbacks);
  camera_move_input_ = new DefaultCameraMoveInput(scene.GetCamera());
  input_system_.AddInputComponent(camera_move_input_);
  return true;
}

void VulkanApp::NextFrame(double deltaTime) {
  // auto tStart = std::chrono::high_resolution_clock::now();
  Render(deltaTime);
  frameCounter++;
}

void VulkanApp::RenderLoop() {
  auto start_time = std::chrono::high_resolution_clock::now();
  auto end_time = std::chrono::high_resolution_clock::now();
  auto diff_time = end_time - start_time;

  // test
  double second = 2.0;

  while (!window_->ShouldClose()) {
    window_->PollEvents();
    if (prepared) {
      auto diffm = std::chrono::duration<double, std::milli>(end_time - start_time).count();
      auto diffsecond = diffm / 1000.0f;
      start_time = std::chrono::high_resolution_clock::now();
      NextFrame(diffsecond);
      end_time = std::chrono::high_resolution_clock::now();

      second -= diffsecond;
      if (second <= 0) {
        // std::cout << "2 seconds!" << std::endl;
        second = 2.0;
      }
    }
  }
  vkDeviceWaitIdle(device);
}

std::string VulkanApp::GetShadersPath() const { return ""; }

void VulkanApp::Prepare() {}

std::string VulkanApp::GetWindowTitle() {
  std::string device(deviceProperties.deviceName);
  std::string windowTitle;
  windowTitle = title + " - " + device;
  if (!settings.overlay) {
    windowTitle += " - " + std::to_string(frameCounter) + " fps";
  }
  return windowTitle;
}

void VulkanApp::HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {}

void VulkanApp::Update(float delta_time) {
  if (!prepared) return;
  if (camera_move_input_) {
    camera_move_input_->Update(delta_time);
  }
}

}  // namespace lvk
