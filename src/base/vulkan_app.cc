#include <cstdlib>
#define NOMINMAX
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <iostream>
#include <ratio>
#include <vector>

#include "imgui.h"
#include "input.h"
#include "lvk_log.h"
#include "lvk_math.h"
#include "vulkan_app.h"
#include "vulkan_context.h"
#include "vulkan_debug.h"
#include "vulkan_device.h"
#include "vulkan_initializers.h"
#include "vulkan_tools.h"
#include "vulkan_ui.h"
#include "window.h"


// for renderdoc
#ifdef ENABLE_RENDERDOC
#include "renderdoc_app.h"
#endif

namespace lvk {
std::vector<const char *> VulkanApp::args;

void DefaultCameraMoveInput::OnDirectionInput(const DirectionInput &di) {
  // DEBUG_LOG("direction input: {}, {}", (int)di.direction, di.scale);
  if (di.direction == kInputDirection::Forward) {
    input_vec_ += camera_->GetForwardVector() * move_speed_ * di.scale;
  } else if (di.direction == kInputDirection::Right) {
    input_vec_ += camera_->GetRightVector() * move_speed_ * di.scale;
  }
}

void DefaultCameraMoveInput::OnRotationInput(const DirectionInput &ri) {
  // DEBUG_LOG("rotation input: {}, {}", (int)ri.direction, ri.scale);
  if (ri.direction == kInputDirection::Forward) {
    rotation_vec_ += vector::RIGHT * rotation_speed_ * ri.scale;
  } else if (ri.direction == kInputDirection::Right) {
    rotation_vec_ += vector::UP * rotation_speed_ * ri.scale;
  }
}

void DefaultCameraMoveInput::OnMouseClick(MouseButton button, MouseState action) {
  if (button == MouseButton::Left) {
    auto tmp = (action == MouseState::Down);
    if (tmp != mouse_left_down_) {
      DEBUG_LOG("mouse_left_down: {} -> {}", mouse_left_down_, tmp);
      mouse_left_down_ = tmp;
    }
  }

  if (mouse_left_down_) {
    init_position_ = false;
  }
}

void DefaultCameraMoveInput::OnMouseMove(double x, double y) {
  vec2f current_pos(x, y);
  if (mouse_left_down_) {
    if (!init_position_) {
      last_mouse_position_ = current_pos;
      init_position_ = true;
      return;
    }

    // DEBUG_LOG("last_mouse_position: {}", glm::to_string(last_mouse_position_));
    // DEBUG_LOG("current_pos: {}", glm::to_string(current_pos));

    mouse_move_delta_ += current_pos - last_mouse_position_;
    last_mouse_position_ = current_pos;

    // DEBUG_LOG("mouse_move_delta_: {}", glm::to_string(mouse_move_delta_));
  }
}

void DefaultCameraMoveInput::OnKey(KeyCode key, KeyState action) {
  switch (key) {
    case KeyCode::W:
      key_state_[0] = action;
      break;
    case KeyCode::S:
      key_state_[1] = action;
      break;
    case KeyCode::A:
      key_state_[2] = action;
      break;
    case KeyCode::D:
      key_state_[3] = action;
      break;
    default:
      break;
  }
}

void DefaultCameraMoveInput::UpdateMouseLookMode(float delta_time) {
  if (!mouse_left_down_) {
    return;
  }

  bool a = abs(mouse_move_delta_.x) > abs(mouse_move_delta_.y);

  if (a) {
    auto location = camera_->GetLocation();
    auto angle = mouse_move_delta_.x * 0.2;
    auto new_location = matrix::RotateVector(location, angle, vec3f(0, 1, 0));
    camera_->SetLocation(new_location);
    auto rot_mat = matrix::MakeFromLookAt(new_location, vec3f(0.0f, 0.0f, 0.0f));
    camera_->SetRotationMatrix(rot_mat);
    mouse_move_delta_.x = 0;
  } else {
    auto location = camera_->GetLocation();
    auto angle = mouse_move_delta_.y * 0.2;
    auto new_location = matrix::RotateVector(location, angle, vec3f(1, 0, 0));
    camera_->SetLocation(new_location);
    auto rot_mat = matrix::MakeFromLookAt(new_location, vec3f(0.0f, 0.0f, 0.0f));
    camera_->SetRotationMatrix(rot_mat);
    mouse_move_delta_.y = 0;
  }
}

void DefaultCameraMoveInput::Update(float delta_time) {
#if 0
  camera_->SetLocation(camera_->GetLocation() + input_vec_);
  // DEBUG_LOG("Camera Location: {}", glm::to_string(camera_->GetLocation()));
  input_vec_ = ZERO_VECTOR;
  camera_->SetRotation(camera_->GetRotation() + rotation_vec_);
  rotation_vec_ = ZERO_VECTOR;
#endif

#define ISDOWN(k) ((k) == KeyState::Down || (k) == KeyState::Hold)

  vec3f move_vec = ZERO_VECTOR;
  if (ISDOWN(key_state_[0])) {
    move_vec += camera_->GetForwardVector() * move_speed_ * delta_time;
  }
  if (ISDOWN(key_state_[1])) {
    move_vec += camera_->GetForwardVector() * move_speed_ * delta_time * (-1.0f);
  }
  if (ISDOWN(key_state_[2])) {
    move_vec += camera_->GetRightVector() * move_speed_ * delta_time * (-1.0f);
  }
  if (ISDOWN(key_state_[3])) {
    move_vec += camera_->GetRightVector() * move_speed_ * delta_time;
  }
  camera_->SetLocation(camera_->GetLocation() + move_vec);

  UpdateMouseLookMode(delta_time);

#if 0
  bool a = mouse_move_delta_.x > mouse_move_delta_.y;

  if (1 && glm::epsilonNotEqual(mouse_move_delta_.x, 0.0f, 0.00001f)) {
#if 0
    auto rotation = camera_->GetRotation();
    rotation.y += mouse_move_delta_.x * 0.125;
    mouse_move_delta_.x = 0;
    camera_->SetRotation(rotation);
#endif
    auto location = camera_->GetLocation();
    auto angle = mouse_move_delta_.x * 0.0125f;
    auto new_location = glm::rotate(location, angle, vec3f(0, 1, 0));
    camera_->SetLocation(new_location);
    auto rotation = camera_->GetRotation() + vec3f(0, glm::degrees(angle), 0);
    camera_->SetRotation(rotation);
    mouse_move_delta_.x = 0;
  }

  if (0 && glm::epsilonNotEqual(mouse_move_delta_.y, 0.0f, 0.00001f)) {
#if 0
    auto rotation = camera_->GetRotation();
    rotation.x += mouse_move_delta_.y * 0.125;
    mouse_move_delta_.y = 0;
    camera_->SetRotation(rotation);
#endif
    auto location = camera_->GetLocation();
    auto angle = mouse_move_delta_.y * 0.0125f;
    auto new_location = glm::rotate(location, angle, vec3f(1, 0, 0));
    camera_->SetLocation(new_location);
    auto rotation = camera_->GetRotation() + vec3f(glm::degrees(angle), 0, 0);
    camera_->SetRotation(rotation);
    mouse_move_delta_.y = 0;
  }
#endif
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

  enabledDeviceExtensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
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
      .OnMouseMove = [this](double x, double y) { input_system_.OnMouseMove(x, y); },
      .OnMouseClick = [this](int button, int action, int mod) { input_system_.OnMouseClick(button, action); },
  };

  window_->SetEventCallbacks(callbacks);
  camera_move_input_ = new DefaultCameraMoveInput(scene.GetCamera());
  input_system_.AddInputComponent(camera_move_input_);

  ui_render_wrapper_ = new VulkanUIRenderWrapper(&ui_, width, height);
  context_->AddRenderComponent(ui_render_wrapper_);

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

void VulkanApp::Prepare() {
  InitScene();
  context_->CreateVulkanScene(&scene, vulkanDevice);
  context_->BuildCommandBuffers(&scene);
  prepared = true;
}

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

void VulkanApp::Render(double deltaTime) {
  if (!prepared) return;
  Update(deltaTime);
  context_->Draw(&scene);
  UpdateOverlay(&scene);
}

void VulkanApp::Update(float delta_time) {
  if (!prepared) return;
  if (camera_move_input_) {
    camera_move_input_->Update(delta_time);
  }
}

void VulkanApp::UpdateOverlay(Scene *scene) {
  // if (!settings.overlay)
  // 	return;

  // The overlay does not need to be updated with each frame, so we limit the update rate
  // Not only does this save performance but it also makes display of fast changig values like fps more stable
  // ui.updateTimer -= frameTimer;
  // if (ui.updateTimer >= 0.0f) {
  // 	return;
  // }
  // Update at max. rate of 30 fps
  ui_.updateTimer = 1.0f / 30.0f;

  ImGuiIO &io = ImGui::GetIO();

  io.DisplaySize = ImVec2((float)width, (float)height);
  io.DeltaTime = 1.0f / 30.0f;  // frameTimer;

  io.MousePos = ImVec2(input_system_.GetMousePosition().x, input_system_.GetMousePosition().y);
  io.MouseDown[0] = input_system_.GetMouseState(0);
  io.MouseDown[1] = input_system_.GetMouseState(1);
  io.MouseDown[2] = input_system_.GetMouseState(2);

  ImGui::NewFrame();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
  ImGui::SetNextWindowPos(ImVec2(10 * ui_.scale, 10 * ui_.scale));
  ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
  ImGui::Begin("Vulkan Example", nullptr,
               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
  ImGui::TextUnformatted("Title Here!");
  ImGui::TextUnformatted("deviceProperties.deviceName");
  ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / 60), 60);

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 5.0f * ui.scale));
#endif
  ImGui::PushItemWidth(110.0f * ui_.scale);

  SetupUI(&ui_);

  ImGui::PopItemWidth();
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
  ImGui::PopStyleVar();
#endif

  ImGui::End();
  ImGui::PopStyleVar();
  ImGui::Render();

  if (ui_.Update() || ui_.updated) {
    context_->BuildCommandBuffers(scene);
    ui_.updated = false;
  }

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
  if (mouseState.buttons.left) {
    mouseState.buttons.left = false;
  }
#endif
}

void VulkanApp::StartRenderdoc() {
#ifdef ENABLE_RENDERDOC
  RENDERDOC_API_1_1_2 *rdoc_api = NULL;

  // At init, on windows
  if (HMODULE mod = LoadLibrary("C:\\Program Files\\RenderDoc\\renderdoc.dll")) {
    pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
    int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
    assert(ret == 1);
  }

  // To start a frame capture, call StartFrameCapture.
  // You can specify NULL, NULL for the device to capture on if you have only one device and
  // either no windows at all or only one window, and it will capture from that device.
  // See the documentation below for a longer explanation
  if (rdoc_api) {
    rdoc_api->StartFrameCapture(NULL, NULL);
    rdoc_api_ = rdoc_api;
  }
#endif
}

void VulkanApp::EndRenderdoc() {
  // stop the capture
#ifdef ENABLE_RENDERDOC
if (rdoc_api_) {
  ((RENDERDOC_API_1_1_2 *)rdoc_api_)->EndFrameCapture(NULL, NULL);
}
#endif
}

}  // namespace lvk
