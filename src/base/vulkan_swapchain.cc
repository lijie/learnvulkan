#include "vulkan_swapchain.h"

#include <assert.h>

#include "vulkan_tools.h"
#include "window.h"

namespace lvk {

void VulkanSwapchain::InitSurface(void* platformHandle, Window* platformWindow) {
  VkResult err = VK_SUCCESS;

  // Create the os-specific surface
#if defined(VK_USE_PLATFORM_WIN32_KHR)
  // VK_CHECK_RESULT(glfwCreateWindowSurface(instance_, (GLFWwindow *)platformWindow, nullptr, &surface_));
  platformWindow->CreateWindowSurface(instance_, &surface_);

  // VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
  // surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  // surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;
  // surfaceCreateInfo.hwnd = (HWND)platformWindow;
  // err = vkCreateWin32SurfaceKHR(instance_, &surfaceCreateInfo, nullptr,
  //                               &surface_);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
  VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
  surfaceCreateInfo.window = window;
  err = vkCreateAndroidSurfaceKHR(instance, &surfaceCreateInfo, NULL, &surface);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
  VkIOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
  surfaceCreateInfo.pNext = NULL;
  surfaceCreateInfo.flags = 0;
  surfaceCreateInfo.pView = view;
  err = vkCreateIOSSurfaceMVK(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
  VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo = {};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
  surfaceCreateInfo.pNext = NULL;
  surfaceCreateInfo.flags = 0;
  surfaceCreateInfo.pView = view;
  err = vkCreateMacOSSurfaceMVK(instance, &surfaceCreateInfo, NULL, &surface);
#elif defined(_DIRECT2DISPLAY)
  createDirect2DisplaySurface(width, height);
#elif defined(VK_USE_PLATFORM_DIRECTFB_EXT)
  VkDirectFBSurfaceCreateInfoEXT surfaceCreateInfo = {};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_DIRECTFB_SURFACE_CREATE_INFO_EXT;
  surfaceCreateInfo.dfb = dfb;
  surfaceCreateInfo.surface = window;
  err = vkCreateDirectFBSurfaceEXT(instance, &surfaceCreateInfo, nullptr,
                                   &surface);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
  VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = {};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
  surfaceCreateInfo.display = display;
  surfaceCreateInfo.surface = window;
  err = vkCreateWaylandSurfaceKHR(instance, &surfaceCreateInfo, nullptr,
                                  &surface);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
  VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
  surfaceCreateInfo.connection = connection;
  surfaceCreateInfo.window = window;
  err = vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
#elif defined(VK_USE_PLATFORM_HEADLESS_EXT)
  VkHeadlessSurfaceCreateInfoEXT surfaceCreateInfo = {};
  surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;
  PFN_vkCreateHeadlessSurfaceEXT fpCreateHeadlessSurfaceEXT =
      (PFN_vkCreateHeadlessSurfaceEXT)vkGetInstanceProcAddr(
          instance, "vkCreateHeadlessSurfaceEXT");
  if (!fpCreateHeadlessSurfaceEXT) {
    vks::tools::exitFatal(
        "Could not fetch function pointer for the headless extension!", -1);
  }
  err = fpCreateHeadlessSurfaceEXT(instance, &surfaceCreateInfo, nullptr,
                                   &surface);
#endif

  if (err != VK_SUCCESS) {
    tools::ExitFatal("Could not create surface!", err);
  }

  // Get available queue family properties
  uint32_t queueCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueCount, NULL);
  assert(queueCount >= 1);

  std::vector<VkQueueFamilyProperties> queueProps(queueCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueCount,
                                           queueProps.data());

  // Iterate over each queue to learn whether it supports presenting:
  // Find a queue with present support
  // Will be used to present the swap chain images to the windowing system
  std::vector<VkBool32> supportsPresent(queueCount);
  for (uint32_t i = 0; i < queueCount; i++) {
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, i, surface_,
                                         &supportsPresent[i]);
  }

  // Search for a graphics and a present queue in the array of queue
  // families, try to find one that supports both
  uint32_t graphicsQueueNodeIndex = UINT32_MAX;
  uint32_t presentQueueNodeIndex = UINT32_MAX;
  for (uint32_t i = 0; i < queueCount; i++) {
    if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
      if (graphicsQueueNodeIndex == UINT32_MAX) {
        graphicsQueueNodeIndex = i;
      }

      if (supportsPresent[i] == VK_TRUE) {
        graphicsQueueNodeIndex = i;
        presentQueueNodeIndex = i;
        break;
      }
    }
  }
  if (presentQueueNodeIndex == UINT32_MAX) {
    // If there's no queue that supports both present and graphics
    // try to find a separate present queue
    for (uint32_t i = 0; i < queueCount; ++i) {
      if (supportsPresent[i] == VK_TRUE) {
        presentQueueNodeIndex = i;
        break;
      }
    }
  }

  // Exit if either a graphics or a presenting queue hasn't been found
  if (graphicsQueueNodeIndex == UINT32_MAX ||
      presentQueueNodeIndex == UINT32_MAX) {
    tools::ExitFatal("Could not find a graphics and/or presenting queue!", -1);
  }

  // todo : Add support for separate graphics and presenting queue
  if (graphicsQueueNodeIndex != presentQueueNodeIndex) {
    tools::ExitFatal(
        "Separate graphics and presenting queues are not supported yet!", -1);
  }

  queueNodeIndex_ = graphicsQueueNodeIndex;

  // Get list of supported surface formats
  uint32_t formatCount;
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
      physicalDevice_, surface_, &formatCount, NULL));
  assert(formatCount > 0);

  std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
      physicalDevice_, surface_, &formatCount, surfaceFormats.data()));

  // We want to get a format that best suits our needs, so we try to get one
  // from a set of preferred formats Initialize the format to the first one
  // returned by the implementation in case we can't find one of the preffered
  // formats
  VkSurfaceFormatKHR selectedFormat = surfaceFormats[0];
  std::vector<VkFormat> preferredImageFormats = {
      VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
      VK_FORMAT_A8B8G8R8_UNORM_PACK32};

  for (auto& availableFormat : surfaceFormats) {
    if (std::find(preferredImageFormats.begin(), preferredImageFormats.end(),
                  availableFormat.format) != preferredImageFormats.end()) {
      selectedFormat = availableFormat;
      break;
    }
  }

  colorFormat_ = selectedFormat.format;
  colorSpace_ = selectedFormat.colorSpace;
}

void VulkanSwapchain::Connect(VkInstance instance,
                              VkPhysicalDevice physicalDevice,
                              VkDevice device) {
  this->instance_ = instance;
  this->physicalDevice_ = physicalDevice;
  this->device_ = device;
}

void VulkanSwapchain::Create(uint32_t* width, uint32_t* height, bool vsync,
                             bool fullscreen) {
  // Store the current swap chain handle so we can use it later on to ease up
  // recreation
  VkSwapchainKHR oldSwapchain = swapchain_;

  // Get physical device surface properties and formats
  VkSurfaceCapabilitiesKHR surfCaps;
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      physicalDevice_, surface_, &surfCaps));

  // Get available present modes
  uint32_t presentModeCount;
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
      physicalDevice_, surface_, &presentModeCount, NULL));
  assert(presentModeCount > 0);

  std::vector<VkPresentModeKHR> presentModes(presentModeCount);
  VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(
      physicalDevice_, surface_, &presentModeCount, presentModes.data()));

  VkExtent2D swapchainExtent = {};
  // If width (and height) equals the special value 0xFFFFFFFF, the size of the
  // surface will be set by the swapchain
  if (surfCaps.currentExtent.width == (uint32_t)-1) {
    // If the surface size is undefined, the size is set to
    // the size of the images requested.
    swapchainExtent.width = *width;
    swapchainExtent.height = *height;
  } else {
    // If the surface size is defined, the swap chain size must match
    swapchainExtent = surfCaps.currentExtent;
    *width = surfCaps.currentExtent.width;
    *height = surfCaps.currentExtent.height;
  }

  // Select a present mode for the swapchain

  // The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
  // This mode waits for the vertical blank ("v-sync")
  VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

  // If v-sync is not requested, try to find a mailbox mode
  // It's the lowest latency non-tearing present mode available
  if (!vsync) {
    for (size_t i = 0; i < presentModeCount; i++) {
      if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
        swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
      }
      if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
      }
    }
  }

  // Determine the number of images
  uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
#if (defined(VK_USE_PLATFORM_MACOS_MVK) && defined(VK_EXAMPLE_XCODE_GENERATED))
  // SRS - Work around known MoltenVK issue re 2x frame rate when vsync
  // (VK_PRESENT_MODE_FIFO_KHR) enabled
  struct utsname sysInfo;
  uname(&sysInfo);
  // SRS - When vsync is on, use minImageCount when not in fullscreen or when
  // running on Apple Silcon This forces swapchain image acquire frame rate to
  // match display vsync frame rate
  if (vsync && (!fullscreen || strcmp(sysInfo.machine, "arm64") == 0)) {
    desiredNumberOfSwapchainImages = surfCaps.minImageCount;
  }
#endif
  if ((surfCaps.maxImageCount > 0) &&
      (desiredNumberOfSwapchainImages > surfCaps.maxImageCount)) {
    desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
  }

  // Find the transformation of the surface
  VkSurfaceTransformFlagsKHR preTransform;
  if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    // We prefer a non-rotated transform
    preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    preTransform = surfCaps.currentTransform;
  }

  // Find a supported composite alpha format (not all devices support alpha
  // opaque)
  VkCompositeAlphaFlagBitsKHR compositeAlpha =
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  // Simply select the first composite alpha format available
  std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
  };
  for (auto& compositeAlphaFlag : compositeAlphaFlags) {
    if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
      compositeAlpha = compositeAlphaFlag;
      break;
    };
  }

  VkSwapchainCreateInfoKHR swapchainCI = {};
  swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainCI.surface = surface_;
  swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
  swapchainCI.imageFormat = colorFormat_;
  swapchainCI.imageColorSpace = colorSpace_;
  swapchainCI.imageExtent = {swapchainExtent.width, swapchainExtent.height};
  swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
  swapchainCI.imageArrayLayers = 1;
  swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainCI.queueFamilyIndexCount = 0;
  swapchainCI.presentMode = swapchainPresentMode;
  // Setting oldSwapChain to the saved handle of the previous swapchain aids in
  // resource reuse and makes sure that we can still present already acquired
  // images
  swapchainCI.oldSwapchain = oldSwapchain;
  // Setting clipped to VK_TRUE allows the implementation to discard rendering
  // outside of the surface area
  swapchainCI.clipped = VK_TRUE;
  swapchainCI.compositeAlpha = compositeAlpha;

  // Enable transfer source on swap chain images if supported
  if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
    swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  // Enable transfer destination on swap chain images if supported
  if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
    swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  VK_CHECK_RESULT(
      vkCreateSwapchainKHR(device_, &swapchainCI, nullptr, &swapchain_));

  // If an existing swap chain is re-created, destroy the old swap chain
  // This also cleans up all the presentable images
  if (oldSwapchain != VK_NULL_HANDLE) {
    for (uint32_t i = 0; i < imageCount_; i++) {
      vkDestroyImageView(device_, buffers_[i].view, nullptr);
    }
    vkDestroySwapchainKHR(device_, oldSwapchain, nullptr);
  }
  VK_CHECK_RESULT(
      vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount_, NULL));

  // Get the swap chain images
  images_.resize(imageCount_);
  VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount_,
                                          images_.data()));

  // Get the swap chain buffers containing the image and imageview
  buffers_.resize(imageCount_);
  for (uint32_t i = 0; i < imageCount_; i++) {
    VkImageViewCreateInfo colorAttachmentView = {};
    colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorAttachmentView.pNext = NULL;
    colorAttachmentView.format = colorFormat_;
    colorAttachmentView.components = {
        VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A};
    colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorAttachmentView.subresourceRange.baseMipLevel = 0;
    colorAttachmentView.subresourceRange.levelCount = 1;
    colorAttachmentView.subresourceRange.baseArrayLayer = 0;
    colorAttachmentView.subresourceRange.layerCount = 1;
    colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorAttachmentView.flags = 0;

    buffers_[i].image = images_[i];

    colorAttachmentView.image = buffers_[i].image;

    VK_CHECK_RESULT(vkCreateImageView(device_, &colorAttachmentView, nullptr,
                                      &buffers_[i].view));
  }
}

VkResult VulkanSwapchain::AcquireNextImage(VkSemaphore presentCompleteSemaphore,
                                           uint32_t* imageIndex) {
  // By setting timeout to UINT64_MAX we will always wait until the next image
  // has been acquired or an actual error is thrown With that we don't have to
  // handle VK_NOT_READY
  return vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                               presentCompleteSemaphore, (VkFence) nullptr,
                               imageIndex);
}

VkResult VulkanSwapchain::QueuePresent(VkQueue queue, uint32_t imageIndex,
                                       VkSemaphore waitSemaphore) {
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = NULL;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain_;
  presentInfo.pImageIndices = &imageIndex;
  // Check if a wait semaphore has been specified to wait for before presenting
  // the image
  if (waitSemaphore != VK_NULL_HANDLE) {
    presentInfo.pWaitSemaphores = &waitSemaphore;
    presentInfo.waitSemaphoreCount = 1;
  }
  return vkQueuePresentKHR(queue, &presentInfo);
}

void VulkanSwapchain::Cleanup() {
  if (swapchain_ != VK_NULL_HANDLE) {
    for (uint32_t i = 0; i < imageCount_; i++) {
      vkDestroyImageView(device_, buffers_[i].view, nullptr);
    }
  }
  if (surface_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
  }
  surface_ = VK_NULL_HANDLE;
  swapchain_ = VK_NULL_HANDLE;
}
}  // namespace lvk
