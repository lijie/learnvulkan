#include "vulkan_app.h"

#include <array>
#include <iostream>
#include <vector>

#include "vulkan_debug.h"
#include "vulkan_device.h"
#include "vulkan_initializers.h"
#include "vulkan_tools.h"

namespace lvk {
std::vector<const char*> VulkanApp::args;
VkResult VulkanApp::CreateInstance(bool enableValidation) {
  this->settings.validation = enableValidation;

  // Validation can also be forced via a define
#if defined(_VALIDATION)
  this->settings.validation = true;
#endif

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = name.c_str();
  appInfo.pEngineName = name.c_str();
  appInfo.apiVersion = apiVersion;

  std::vector<const char*> instanceExtensions = {VK_KHR_SURFACE_EXTENSION_NAME};

  // Enable surface extensions depending on os
#if defined(_WIN32)
  instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
  instanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(_DIRECT2DISPLAY)
  instanceExtensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
  instanceExtensions.push_back(VK_EXT_DIRECTFB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
  instanceExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
  instanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
  instanceExtensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
  instanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_HEADLESS_EXT)
  instanceExtensions.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
#endif

  // Get extensions supported by the instance and store for later use
  uint32_t extCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
  if (extCount > 0) {
    std::vector<VkExtensionProperties> extensions(extCount);
    if (vkEnumerateInstanceExtensionProperties(
            nullptr, &extCount, &extensions.front()) == VK_SUCCESS) {
      for (VkExtensionProperties extension : extensions) {
        supportedInstanceExtensions.push_back(extension.extensionName);
      }
    }
  }

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
  // SRS - When running on iOS/macOS with MoltenVK, enable
  // VK_KHR_get_physical_device_properties2 if not already enabled by the
  // example (required by VK_KHR_portability_subset)
  if (std::find(enabledInstanceExtensions.begin(),
                enabledInstanceExtensions.end(),
                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) ==
      enabledInstanceExtensions.end()) {
    enabledInstanceExtensions.push_back(
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  }
#endif

  // Enabled requested instance extensions
  if (enabledInstanceExtensions.size() > 0) {
    for (const char* enabledExtension : enabledInstanceExtensions) {
      // Output message if requested extension is not available
      if (std::find(supportedInstanceExtensions.begin(),
                    supportedInstanceExtensions.end(),
                    enabledExtension) == supportedInstanceExtensions.end()) {
        std::cerr << "Enabled instance extension \"" << enabledExtension
                  << "\" is not present at instance level\n";
      }
      instanceExtensions.push_back(enabledExtension);
    }
  }

  VkInstanceCreateInfo instanceCreateInfo = {};
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pNext = NULL;
  instanceCreateInfo.pApplicationInfo = &appInfo;

#if (defined(VK_USE_PLATFORM_IOS_MVK) ||    \
     defined(VK_USE_PLATFORM_MACOS_MVK)) && \
    defined(VK_KHR_portability_enumeration)
  // SRS - When running on iOS/macOS with MoltenVK and
  // VK_KHR_portability_enumeration is defined and supported by the instance,
  // enable the extension and the flag
  if (std::find(supportedInstanceExtensions.begin(),
                supportedInstanceExtensions.end(),
                VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) !=
      supportedInstanceExtensions.end()) {
    instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  }
#endif

  // Enable the debug utils extension if available (e.g. when debugging tools
  // are present)
  if (settings.validation || std::find(supportedInstanceExtensions.begin(),
                                       supportedInstanceExtensions.end(),
                                       VK_EXT_DEBUG_UTILS_EXTENSION_NAME) !=
                                 supportedInstanceExtensions.end()) {
    instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  if (instanceExtensions.size() > 0) {
    instanceCreateInfo.enabledExtensionCount =
        (uint32_t)instanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
  }

  // The VK_LAYER_KHRONOS_validation contains all current validation
  // functionality. Note that on Android this layer requires at least NDK r20
  const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
  if (settings.validation) {
    // Check if this layer is available at instance level
    uint32_t instanceLayerCount;
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
    std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
    vkEnumerateInstanceLayerProperties(&instanceLayerCount,
                                       instanceLayerProperties.data());
    bool validationLayerPresent = false;
    for (VkLayerProperties layer : instanceLayerProperties) {
      if (strcmp(layer.layerName, validationLayerName) == 0) {
        validationLayerPresent = true;
        break;
      }
    }
    if (validationLayerPresent) {
      instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
      instanceCreateInfo.enabledLayerCount = 1;
    } else {
      std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, "
                   "validation is disabled";
    }
  }
  VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

  // If the debug utils extension is present we set up debug functions, so
  // samples an label objects for debugging
  if (std::find(supportedInstanceExtensions.begin(),
                supportedInstanceExtensions.end(),
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME) !=
      supportedInstanceExtensions.end()) {
    debugutils::Setup(instance);
  }

  return result;
}

bool VulkanApp::InitVulkan() {
  VkResult err;

  // Vulkan instance
  err = CreateInstance(settings.validation);
  if (err) {
    tools::ExitFatal(
        "Could not create Vulkan instance : \n" + tools::ErrorString(err), err);
    return false;
  }

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
  vks::android::loadVulkanFunctions(instance);
#endif

  // If requested, we enable the default validation layers for debugging
  if (settings.validation) {
    debug::SetupDebugging(instance);
  }

  // Physical device
  uint32_t gpuCount = 0;
  // Get number of available physical devices
  VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
  if (gpuCount == 0) {
    tools::ExitFatal("No device with Vulkan support found", -1);
    return false;
  }
  // Enumerate devices
  std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
  err = vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());
  if (err) {
    tools::ExitFatal(
        "Could not enumerate physical devices : \n" + tools::ErrorString(err),
        err);
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
  if (commandLineParser.isSet("gpulist")) {
    std::cout << "Available Vulkan devices"
              << "\n";
    for (uint32_t i = 0; i < gpuCount; i++) {
      VkPhysicalDeviceProperties deviceProperties;
      vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
      std::cout << "Device [" << i << "] : " << deviceProperties.deviceName
                << std::endl;
      std::cout << " Type: "
                << tools::PhysicalDeviceTypeString(deviceProperties.deviceType)
                << "\n";
      std::cout << " API: " << (deviceProperties.apiVersion >> 22) << "."
                << ((deviceProperties.apiVersion >> 12) & 0x3ff) << "."
                << (deviceProperties.apiVersion & 0xfff) << "\n";
    }
  }
#endif

  physicalDevice = physicalDevices[selectedDevice];

  // Store properties (including limits), features and memory properties of the
  // physical device (so that examples can check against them)
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
  vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

  // Derived examples can override this to set actual features (based on above
  // readings) to enable for logical device creation
  GetEnabledFeatures();

  // Vulkan device creation
  // This is handled by a separate class that gets a logical device
  // representation and encapsulates functions related to a device
  vulkanDevice = new lvk::VulkanDevice(physicalDevice);

  // Derived examples can enable extensions based on the list of supported
  // extensions read from the physical device
  GetEnabledExtensions();

  VkResult res = vulkanDevice->CreateLogicalDevice(
      enabledFeatures, enabledDeviceExtensions, deviceCreatepNextChain);
  if (res != VK_SUCCESS) {
    tools::ExitFatal(
        "Could not create Vulkan device: \n" + tools::ErrorString(res), res);
    return false;
  }
  device = vulkanDevice->logicalDevice_;

  // Get a graphics queue from the device
  vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices_.graphics, 0,
                   &queue);

  // Find a suitable depth and/or stencil format
  VkBool32 validFormat{false};
  // Sample that make use of stencil will require a depth + stencil format, so
  // we select from a different list
  if (requiresStencil) {
    validFormat =
        tools::GetSupportedDepthStencilFormat(physicalDevice, &depthFormat);
  } else {
    validFormat = tools::GetSupportedDepthFormat(physicalDevice, &depthFormat);
  }
  assert(validFormat);

  swapChain.Connect(instance, physicalDevice, device);

  // Create synchronization objects
  VkSemaphoreCreateInfo semaphoreCreateInfo =
      initializers::SemaphoreCreateInfo();
  // Create a semaphore used to synchronize image presentation
  // Ensures that the image is displayed before we start submitting new commands
  // to the queue
  VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr,
                                    &semaphores.presentComplete));
  // Create a semaphore used to synchronize command submission
  // Ensures that the image is not presented until all commands have been
  // submitted and executed
  VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr,
                                    &semaphores.renderComplete));

  // Set up submit info structure
  // Semaphores will stay the same during application lifetime
  // Command buffer submission info is set by each example
  submitInfo = initializers::SubmitInfo();
  submitInfo.pWaitDstStageMask = &submitPipelineStages;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &semaphores.presentComplete;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &semaphores.renderComplete;

  return true;
}

void VulkanApp::CreateCommandPool() {
  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex();
  cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &cmdPool));
}

void VulkanApp::CreateCommandBuffers() {
  // Create one command buffer for each swap chain image and reuse for rendering
  drawCmdBuffers.resize(swapChain.imageCount());

  VkCommandBufferAllocateInfo cmdBufAllocateInfo =
      initializers::CommandBufferAllocateInfo(
          cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          static_cast<uint32_t>(drawCmdBuffers.size()));

  VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo,
                                           drawCmdBuffers.data()));
}

void VulkanApp::CreateSynchronizationPrimitives() {
  // Wait fences to sync command buffer access
  VkFenceCreateInfo fenceCreateInfo =
      initializers::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
  waitFences.resize(drawCmdBuffers.size());
  for (auto& fence : waitFences) {
    VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
  }
}

void VulkanApp::SetupDepthStencil() {
  VkImageCreateInfo imageCI{};
  imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCI.imageType = VK_IMAGE_TYPE_2D;
  imageCI.format = depthFormat;
  imageCI.extent = {width, height, 1};
  imageCI.mipLevels = 1;
  imageCI.arrayLayers = 1;
  imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VK_CHECK_RESULT(
      vkCreateImage(device, &imageCI, nullptr, &depthStencil.image));
  VkMemoryRequirements memReqs{};
  vkGetImageMemoryRequirements(device, depthStencil.image, &memReqs);

  VkMemoryAllocateInfo memAllloc{};
  memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memAllloc.allocationSize = memReqs.size;
  memAllloc.memoryTypeIndex = vulkanDevice->GetMemoryType(
      memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(
      vkAllocateMemory(device, &memAllloc, nullptr, &depthStencil.mem));
  VK_CHECK_RESULT(
      vkBindImageMemory(device, depthStencil.image, depthStencil.mem, 0));

  VkImageViewCreateInfo imageViewCI{};
  imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewCI.image = depthStencil.image;
  imageViewCI.format = depthFormat;
  imageViewCI.subresourceRange.baseMipLevel = 0;
  imageViewCI.subresourceRange.levelCount = 1;
  imageViewCI.subresourceRange.baseArrayLayer = 0;
  imageViewCI.subresourceRange.layerCount = 1;
  imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  // Stencil aspect should only be set on depth + stencil formats
  // (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
  if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
    imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }
  VK_CHECK_RESULT(
      vkCreateImageView(device, &imageViewCI, nullptr, &depthStencil.view));
}

void VulkanApp::SetupRenderPass() {
  std::array<VkAttachmentDescription, 2> attachments = {};
  // Color attachment
  attachments[0].format = swapChain.colorFormat_;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  // Depth attachment
  attachments[1].format = depthFormat;
  attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorReference = {};
  colorReference.attachment = 0;
  colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthReference = {};
  depthReference.attachment = 1;
  depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpassDescription = {};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount = 1;
  subpassDescription.pColorAttachments = &colorReference;
  subpassDescription.pDepthStencilAttachment = &depthReference;
  subpassDescription.inputAttachmentCount = 0;
  subpassDescription.pInputAttachments = nullptr;
  subpassDescription.preserveAttachmentCount = 0;
  subpassDescription.pPreserveAttachments = nullptr;
  subpassDescription.pResolveAttachments = nullptr;

  // Subpass dependencies for layout transitions
  std::array<VkSubpassDependency, 2> dependencies;

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                 VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
  dependencies[0].dependencyFlags = 0;

  dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].dstSubpass = 0;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].srcAccessMask = 0;
  dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                  VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  dependencies[1].dependencyFlags = 0;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDescription;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  VK_CHECK_RESULT(
      vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

void VulkanApp::CreatePipelineCache() {
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo,
                                        nullptr, &pipelineCache));
}

void VulkanApp::SetupFrameBuffer() {
  VkImageView attachments[2];

  // Depth/Stencil attachment is the same for all frame buffers
  attachments[1] = depthStencil.view;

  VkFramebufferCreateInfo frameBufferCreateInfo = {};
  frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  frameBufferCreateInfo.pNext = NULL;
  frameBufferCreateInfo.renderPass = renderPass;
  frameBufferCreateInfo.attachmentCount = 2;
  frameBufferCreateInfo.pAttachments = attachments;
  frameBufferCreateInfo.width = width;
  frameBufferCreateInfo.height = height;
  frameBufferCreateInfo.layers = 1;

  // Create frame buffers for every swap chain image
  frameBuffers.resize(swapChain.imageCount_);
  for (uint32_t i = 0; i < frameBuffers.size(); i++) {
    attachments[0] = swapChain.buffers_[i].view;
    VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr,
                                        &frameBuffers[i]));
  }
}

void VulkanApp::NextFrame() {
  auto tStart = std::chrono::high_resolution_clock::now();
#if 0
  if (viewUpdated) {
    viewUpdated = false;
    viewChanged();
  }
#endif

  Render();
  frameCounter++;
#if 0
  auto tEnd = std::chrono::high_resolution_clock::now();
#if (defined(VK_USE_PLATFORM_IOS_MVK) ||    \
     (defined(VK_USE_PLATFORM_MACOS_MVK) && \
      !defined(VK_EXAMPLE_XCODE_GENERATED)))
  // SRS - Calculate tDiff as time between frames vs. rendering time for
  // iOS/macOS displayLink-driven examples project
  auto tDiff =
      std::chrono::duration<double, std::milli>(tEnd - tPrevEnd).count();
#else
  auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
#endif
  frameTimer = (float)tDiff / 1000.0f;
  camera.update(frameTimer);
  if (camera.moving()) {
    viewUpdated = true;
  }
  // Convert to clamped timer value
  if (!paused) {
    timer += timerSpeed * frameTimer;
    if (timer > 1.0) {
      timer -= 1.0f;
    }
  }
  float fpsTimer =
      (float)(std::chrono::duration<double, std::milli>(tEnd - lastTimestamp)
                  .count());
  if (fpsTimer > 1000.0f) {
    lastFPS = static_cast<uint32_t>((float)frameCounter * (1000.0f / fpsTimer));
#if defined(_WIN32)
    if (!settings.overlay) {
      std::string windowTitle = GetWindowTitle();
      SetWindowText(window, windowTitle.c_str());
    }
#endif
    frameCounter = 0;
    lastTimestamp = tEnd;
  }
  tPrevEnd = tEnd;

  // TODO: Cap UI overlay update rates
  // updateOverlay();
#endif
}

void VulkanApp::RenderLoop() {
#if 0
// SRS - for non-apple plaforms, handle benchmarking here within
// VulkanExampleBase::renderLoop()
//     - for macOS, handle benchmarking within NSApp rendering loop via
//     displayLinkOutputCb()
#if !(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
  if (benchmark.active) {
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
    while (!configured) wl_display_dispatch(display);
    while (wl_display_prepare_read(display) != 0)
      wl_display_dispatch_pending(display);
    wl_display_flush(display);
    wl_display_read_events(display);
    wl_display_dispatch_pending(display);
#endif

    benchmark.run([=] { render(); }, vulkanDevice->properties);
    vkDeviceWaitIdle(device);
    if (benchmark.filename != "") {
      benchmark.saveResults();
    }
    return;
  }
#endif
#endif

  destWidth = width;
  destHeight = height;
  lastTimestamp = std::chrono::high_resolution_clock::now();
  tPrevEnd = lastTimestamp;
#if defined(_WIN32)
  MSG msg;
  bool quitMessageReceived = false;
  while (!quitMessageReceived) {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
      if (msg.message == WM_QUIT) {
        quitMessageReceived = true;
        break;
      }
    }
    if (prepared && !IsIconic(window)) {
      NextFrame();
    }
  }
#endif
  // Flush device to make sure all resources can be freed
  if (device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device);
  }
}

std::string VulkanApp::GetShadersPath() const { return ""; }

VkPipelineShaderStageCreateInfo VulkanApp::LoadShader(
    std::string fileName, VkShaderStageFlagBits stage) {
  VkPipelineShaderStageCreateInfo shaderStage = {};
  shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStage.stage = stage;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
  shaderStage.module = vks::tools::loadShader(
      androidApp->activity->assetManager, fileName.c_str(), device);
#else
  shaderStage.module = tools::LoadShader(fileName.c_str(), device);
#endif
  shaderStage.pName = "main";
  assert(shaderStage.module != VK_NULL_HANDLE);
  shaderModules.push_back(shaderStage.module);
  return shaderStage;
}

void VulkanApp::PrepareFrame() {
  // Acquire the next image from the swap chain
  VkResult result =
      swapChain.AcquireNextImage(semaphores.presentComplete, &currentBuffer);
  // Recreate the swapchain if it's no longer compatible with the surface
  // (OUT_OF_DATE) SRS - If no longer optimal (VK_SUBOPTIMAL_KHR), wait until
  // submitFrame() in case number of swapchain images will change on resize
  if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      // windowResize();
    }
    return;
  } else {
    VK_CHECK_RESULT(result);
  }
}

void VulkanApp::SubmitFrame() {
  VkResult result =
      swapChain.QueuePresent(queue, currentBuffer, semaphores.renderComplete);
  // Recreate the swapchain if it's no longer compatible with the surface
  // (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
  if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
    // windowResize();
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      return;
    }
  } else {
    VK_CHECK_RESULT(result);
  }
  VK_CHECK_RESULT(vkQueueWaitIdle(queue));
}

void VulkanApp::Prepare() {
  InitSwapchain();
  CreateCommandPool();
  SetupSwapchain();
  CreateCommandBuffers();
  CreateSynchronizationPrimitives();
  SetupDepthStencil();
  SetupRenderPass();
  CreatePipelineCache();
  SetupFrameBuffer();
#if 0
  settings.overlay = settings.overlay && (!benchmark.active);
  if (settings.overlay) {
    UIOverlay.device = vulkanDevice;
    UIOverlay.queue = queue;
    UIOverlay.shaders = {
        loadShader(getShadersPath() + "base/uioverlay.vert.spv",
                   VK_SHADER_STAGE_VERTEX_BIT),
        loadShader(getShadersPath() + "base/uioverlay.frag.spv",
                   VK_SHADER_STAGE_FRAGMENT_BIT),
    };
    UIOverlay.prepareResources();
    UIOverlay.preparePipeline(pipelineCache, renderPass, swapChain.colorFormat,
                              depthFormat);
  }
#endif
}

void VulkanApp::InitSwapchain() {
  swapChain.InitSurface(windowInstance, window);
}

void VulkanApp::SetupSwapchain() {
  swapChain.Create(&width, &height, settings.vsync, settings.fullscreen);
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

void VulkanApp::HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam,
                               LPARAM lParam) {
  switch (uMsg) {
    case WM_CLOSE:
      prepared = false;
      DestroyWindow(hWnd);
      PostQuitMessage(0);
      break;
    case WM_PAINT:
      ValidateRect(window, NULL);
      break;
#if 0
    case WM_KEYDOWN:
      switch (wParam) {
        case KEY_P:
          paused = !paused;
          break;
        case KEY_F1:
          UIOverlay.visible = !UIOverlay.visible;
          UIOverlay.updated = true;
          break;
        case KEY_ESCAPE:
          PostQuitMessage(0);
          break;
      }

      if (camera.type == Camera::firstperson) {
        switch (wParam) {
          case KEY_W:
            camera.keys.up = true;
            break;
          case KEY_S:
            camera.keys.down = true;
            break;
          case KEY_A:
            camera.keys.left = true;
            break;
          case KEY_D:
            camera.keys.right = true;
            break;
        }
      }

      keyPressed((uint32_t)wParam);
      break;
    case WM_KEYUP:
      if (camera.type == Camera::firstperson) {
        switch (wParam) {
          case KEY_W:
            camera.keys.up = false;
            break;
          case KEY_S:
            camera.keys.down = false;
            break;
          case KEY_A:
            camera.keys.left = false;
            break;
          case KEY_D:
            camera.keys.right = false;
            break;
        }
      }
      break;
    case WM_LBUTTONDOWN:
      mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
      mouseButtons.left = true;
      break;
    case WM_RBUTTONDOWN:
      mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
      mouseButtons.right = true;
      break;
    case WM_MBUTTONDOWN:
      mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
      mouseButtons.middle = true;
      break;
    case WM_LBUTTONUP:
      mouseButtons.left = false;
      break;
    case WM_RBUTTONUP:
      mouseButtons.right = false;
      break;
    case WM_MBUTTONUP:
      mouseButtons.middle = false;
      break;
    case WM_MOUSEWHEEL: {
      short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
      camera.translate(glm::vec3(0.0f, 0.0f, (float)wheelDelta * 0.005f));
      viewUpdated = true;
      break;
    }
    case WM_MOUSEMOVE: {
      handleMouseMove(LOWORD(lParam), HIWORD(lParam));
      break;
    }
    case WM_SIZE:
      if ((prepared) && (wParam != SIZE_MINIMIZED)) {
        if ((resizing) ||
            ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED))) {
          destWidth = LOWORD(lParam);
          destHeight = HIWORD(lParam);
          windowResize();
        }
      }
      break;
    case WM_GETMINMAXINFO: {
      LPMINMAXINFO minMaxInfo = (LPMINMAXINFO)lParam;
      minMaxInfo->ptMinTrackSize.x = 64;
      minMaxInfo->ptMinTrackSize.y = 64;
      break;
    }
    case WM_ENTERSIZEMOVE:
      resizing = true;
      break;
    case WM_EXITSIZEMOVE:
      resizing = false;
      break;
#endif
  }

  // OnHandleMessage(hWnd, uMsg, wParam, lParam);
}

HWND VulkanApp::SetupWindow(HINSTANCE hinstance, WNDPROC wndproc) {
  this->windowInstance = hinstance;

  WNDCLASSEX wndClass;

  wndClass.cbSize = sizeof(WNDCLASSEX);
  wndClass.style = CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc = wndproc;
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.hInstance = hinstance;
  wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wndClass.lpszMenuName = NULL;
  wndClass.lpszClassName = name.c_str();
  wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

  if (!RegisterClassEx(&wndClass)) {
    std::cout << "Could not register window class!\n";
    fflush(stdout);
    exit(1);
  }

  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  if (settings.fullscreen) {
    if ((width != (uint32_t)screenWidth) &&
        (height != (uint32_t)screenHeight)) {
      DEVMODE dmScreenSettings;
      memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
      dmScreenSettings.dmSize = sizeof(dmScreenSettings);
      dmScreenSettings.dmPelsWidth = width;
      dmScreenSettings.dmPelsHeight = height;
      dmScreenSettings.dmBitsPerPel = 32;
      dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
      if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) !=
          DISP_CHANGE_SUCCESSFUL) {
        if (MessageBox(
                NULL, "Fullscreen Mode not supported!\n Switch to window mode?",
                "Error", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
          settings.fullscreen = false;
        } else {
          return nullptr;
        }
      }
      screenWidth = width;
      screenHeight = height;
    }
  }

  DWORD dwExStyle;
  DWORD dwStyle;

  if (settings.fullscreen) {
    dwExStyle = WS_EX_APPWINDOW;
    dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
  } else {
    dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
  }

  RECT windowRect;
  windowRect.left = 0L;
  windowRect.top = 0L;
  windowRect.right = settings.fullscreen ? (long)screenWidth : (long)width;
  windowRect.bottom = settings.fullscreen ? (long)screenHeight : (long)height;

  AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

  std::string windowTitle = GetWindowTitle();
  window = CreateWindowEx(0, name.c_str(), windowTitle.c_str(),
                          dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0,
                          windowRect.right - windowRect.left,
                          windowRect.bottom - windowRect.top, NULL, NULL,
                          hinstance, NULL);

  if (!settings.fullscreen) {
    // Center on screen
    uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
    uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
    SetWindowPos(window, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
  }

  if (!window) {
    printf("Could not create window!\n");
    fflush(stdout);
    return nullptr;
  }

  ShowWindow(window, SW_SHOW);
  SetForegroundWindow(window);
  SetFocus(window);

  return window;
}
}  // namespace lvk
