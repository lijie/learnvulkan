#include "vulkan_app.h"

#include <iostream>
#include <vector>

#include "vulkan_debug.h"
#include "vulkan_device.h"
#include "vulkan_tools.h"
#include "vulkan_initializers.h"

namespace lvk {
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
    validFormat = tools::GetSupportedDepthStencilFormat(physicalDevice,
                                                             &depthFormat);
  } else {
    validFormat =
        tools::GetSupportedDepthFormat(physicalDevice, &depthFormat);
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
}  // namespace lvk