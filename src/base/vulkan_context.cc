#include "vulkan_context.h"

#include <iostream>
#include <vector>

#include "vulkan_debug.h"
#include "vulkan_tools.h"

namespace lvk {

VulkanContext::VulkanContext() { VK_CHECK_RESULT(CreateInstance(true)); }

VulkanContext::~VulkanContext() {}

VkResult VulkanContext::CreateInstance(bool enableValidation) {
  // this->settings.validation = enableValidation;

  // Validation can also be forced via a define
#if defined(_VALIDATION)
  this->settings.validation = true;
#endif

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Learn Vulkan";
  appInfo.pEngineName = "Learn Vulkan";
  appInfo.apiVersion = VK_API_VERSION_1_0;

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
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, &extensions.front()) == VK_SUCCESS) {
      for (VkExtensionProperties extension : extensions) {
        supportedInstanceExtensions.push_back(extension.extensionName);
      }
    }
  }

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
  // SRS - When running on iOS/macOS with MoltenVK, enable
  // VK_KHR_get_physical_device_properties2 if not already enabled by the
  // example (required by VK_KHR_portability_subset)
  if (std::find(enabledInstanceExtensions.begin(), enabledInstanceExtensions.end(),
                VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == enabledInstanceExtensions.end()) {
    enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  }
#endif

  // Enabled requested instance extensions
  if (enabledInstanceExtensions.size() > 0) {
    for (const char* enabledExtension : enabledInstanceExtensions) {
      // Output message if requested extension is not available
      if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), enabledExtension) ==
          supportedInstanceExtensions.end()) {
        std::cerr << "Enabled instance extension \"" << enabledExtension << "\" is not present at instance level\n";
      }
      instanceExtensions.push_back(enabledExtension);
    }
  }

  VkInstanceCreateInfo instanceCreateInfo = {};
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pNext = NULL;
  instanceCreateInfo.pApplicationInfo = &appInfo;

#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)) && defined(VK_KHR_portability_enumeration)
  // SRS - When running on iOS/macOS with MoltenVK and
  // VK_KHR_portability_enumeration is defined and supported by the instance,
  // enable the extension and the flag
  if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(),
                VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) != supportedInstanceExtensions.end()) {
    instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  }
#endif

  // Enable the debug utils extension if available (e.g. when debugging tools
  // are present)
  if (enableValidation || std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(),
                                    VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != supportedInstanceExtensions.end()) {
    instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  if (instanceExtensions.size() > 0) {
    instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
  }

  // The VK_LAYER_KHRONOS_validation contains all current validation
  // functionality. Note that on Android this layer requires at least NDK r20
  const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
  if (enableValidation) {
    // Check if this layer is available at instance level
    uint32_t instanceLayerCount;
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
    std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
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
  VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance_);

  // If the debug utils extension is present we set up debug functions, so
  // samples an label objects for debugging
  if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(),
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME) != supportedInstanceExtensions.end()) {
    debugutils::Setup(instance_);
  }

  return result;
}

}  // namespace lvk