#include "vulkan_device.h"

#include <assert.h>

#include <iostream>
#include <stdexcept>

#include "vulkan_buffer.h"
#include "vulkan_initializers.h"
#include "vulkan_tools.h"

namespace lvk {
VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice) {
  assert(physicalDevice);
  this->vkPhysicalDevice_ = physicalDevice;

  // Store Properties features, limits and properties of the physical device for
  // later use Device properties also contain limits and sparse properties
  vkGetPhysicalDeviceProperties(physicalDevice, &properties_);
  // Features should be checked by the examples before using them
  vkGetPhysicalDeviceFeatures(physicalDevice, &features_);
  // Memory properties are used regularly for creating all kinds of buffers
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties_);
  // Queue family properties, used for setting up requested queues upon device
  // creation
  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);
  assert(queueFamilyCount > 0);
  queueFamilyProperties_.resize(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilyProperties_.data());

  // Get list of supported extensions
  uint32_t extCount = 0;
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount,
                                       nullptr);
  if (extCount > 0) {
    std::vector<VkExtensionProperties> extensions(extCount);
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount,
                                             &extensions.front()) ==
        VK_SUCCESS) {
      for (auto ext : extensions) {
        supportedExtensions_.push_back(ext.extensionName);
      }
    }
  }
}

uint32_t VulkanDevice::GetMemoryType(uint32_t typeBits,
                                     VkMemoryPropertyFlags properties,
                                     VkBool32 *memTypeFound) const {
  for (uint32_t i = 0; i < memoryProperties_.memoryTypeCount; i++) {
    if ((typeBits & 1) == 1) {
      if ((memoryProperties_.memoryTypes[i].propertyFlags & properties) ==
          properties) {
        if (memTypeFound) {
          *memTypeFound = true;
        }
        return i;
      }
    }
    typeBits >>= 1;
  }

  if (memTypeFound) {
    *memTypeFound = false;
    return 0;
  } else {
    throw std::runtime_error("Could not find a matching memory type");
  }
}

uint32_t VulkanDevice::GetQueueFamilyIndex(VkQueueFlags queueFlags) const {
  // Dedicated queue for compute
  // Try to find a queue family index that supports compute but not graphics
  if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags) {
    for (uint32_t i = 0;
         i < static_cast<uint32_t>(queueFamilyProperties_.size()); i++) {
      if ((queueFamilyProperties_[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
          ((queueFamilyProperties_[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ==
           0)) {
        return i;
      }
    }
  }

  // Dedicated queue for transfer
  // Try to find a queue family index that supports transfer but not graphics
  // and compute
  if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags) {
    for (uint32_t i = 0;
         i < static_cast<uint32_t>(queueFamilyProperties_.size()); i++) {
      if ((queueFamilyProperties_[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
          ((queueFamilyProperties_[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ==
           0) &&
          ((queueFamilyProperties_[i].queueFlags & VK_QUEUE_COMPUTE_BIT) ==
           0)) {
        return i;
      }
    }
  }

  // For other queue types or if no separate compute queue is present, return
  // the first one to support the requested flags
  for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties_.size());
       i++) {
    if ((queueFamilyProperties_[i].queueFlags & queueFlags) == queueFlags) {
      return i;
    }
  }

  throw std::runtime_error("Could not find a matching queue family index");
}

VkResult VulkanDevice::CreateLogicalDevice(
    VkPhysicalDeviceFeatures enabledFeatures,
    const std::vector<const char *> &enabledExtensions, void *pNextChain,
    bool useSwapChain, VkQueueFlags requestedQueueTypes) {
  // Desired queues need to be requested upon logical device creation
  // Due to differing queue family configurations of Vulkan implementations this
  // can be a bit tricky, especially if the application requests different queue
  // types

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

  // Get queue family indices for the requested queue family types
  // Note that the indices may overlap depending on the implementation

  const float defaultQueuePriority(0.0f);

  // Graphics queue
  if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT) {
    queueFamilyIndices_.graphics = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = queueFamilyIndices_.graphics;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &defaultQueuePriority;
    queueCreateInfos.push_back(queueInfo);
  } else {
    queueFamilyIndices_.graphics = 0;
  }

  // Dedicated compute queue
  if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT) {
    queueFamilyIndices_.compute = GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
    if (queueFamilyIndices_.compute != queueFamilyIndices_.graphics) {
      // If compute family index differs, we need an additional queue create
      // info for the compute queue
      VkDeviceQueueCreateInfo queueInfo{};
      queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueInfo.queueFamilyIndex = queueFamilyIndices_.compute;
      queueInfo.queueCount = 1;
      queueInfo.pQueuePriorities = &defaultQueuePriority;
      queueCreateInfos.push_back(queueInfo);
    }
  } else {
    // Else we use the same queue
    queueFamilyIndices_.compute = queueFamilyIndices_.graphics;
  }

  // Dedicated transfer queue
  if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT) {
    queueFamilyIndices_.transfer = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
    if ((queueFamilyIndices_.transfer != queueFamilyIndices_.graphics) &&
        (queueFamilyIndices_.transfer != queueFamilyIndices_.compute)) {
      // If transfer family index differs, we need an additional queue create
      // info for the transfer queue
      VkDeviceQueueCreateInfo queueInfo{};
      queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueInfo.queueFamilyIndex = queueFamilyIndices_.transfer;
      queueInfo.queueCount = 1;
      queueInfo.pQueuePriorities = &defaultQueuePriority;
      queueCreateInfos.push_back(queueInfo);
    }
  } else {
    // Else we use the same queue
    queueFamilyIndices_.transfer = queueFamilyIndices_.graphics;
  }

  // Create the logical device representation
  std::vector<const char *> deviceExtensions(enabledExtensions);
  if (useSwapChain) {
    // If the device will be used for presenting to a display via a swapchain we
    // need to request the swapchain extension
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  }

  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  ;
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

  // If a pNext(Chain) has been passed, we need to add it to the device creation
  // info
  VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
  if (pNextChain) {
    physicalDeviceFeatures2.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physicalDeviceFeatures2.features = enabledFeatures;
    physicalDeviceFeatures2.pNext = pNextChain;
    deviceCreateInfo.pEnabledFeatures = nullptr;
    deviceCreateInfo.pNext = &physicalDeviceFeatures2;
  }

#if (defined(VK_USE_PLATFORM_IOS_MVK) ||    \
     defined(VK_USE_PLATFORM_MACOS_MVK)) && \
    defined(VK_KHR_portability_subset)
  // SRS - When running on iOS/macOS with MoltenVK and VK_KHR_portability_subset
  // is defined and supported by the device, enable the extension
  if (extensionSupported(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) {
    deviceExtensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
  }
#endif

  if (deviceExtensions.size() > 0) {
    for (const char *enabledExtension : deviceExtensions) {
      if (!ExtensionSupported(enabledExtension)) {
        std::cerr << "Enabled device extension \"" << enabledExtension
                  << "\" is not present at device level\n";
      }
    }

    deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
  }

  this->enabledFeatures_ = enabledFeatures;

  VkResult result = vkCreateDevice(vkPhysicalDevice_, &deviceCreateInfo,
                                   nullptr, &logicalDevice_);
  if (result != VK_SUCCESS) {
    return result;
  }

  // Create a default command pool for graphics command buffers
  commandPool_ = CreateCommandPool(queueFamilyIndices_.graphics);

  return result;
}

VkResult VulkanDevice::CreateBuffer(VkBufferUsageFlags usageFlags,
                                    VkMemoryPropertyFlags memoryPropertyFlags,
                                    VkDeviceSize size, VkBuffer *buffer,
                                    VkDeviceMemory *memory, const void *data) {
  // Create the buffer handle
  VkBufferCreateInfo bufferCreateInfo =
      initializers::BufferCreateInfo(usageFlags, size);
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VK_CHECK_RESULT(
      vkCreateBuffer(logicalDevice_, &bufferCreateInfo, nullptr, buffer));

  // Create the memory backing up the buffer handle
  VkMemoryRequirements memReqs;
  VkMemoryAllocateInfo memAlloc = initializers::MemoryAllocateInfo();
  vkGetBufferMemoryRequirements(logicalDevice_, *buffer, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  // Find a memory type index that fits the properties of the buffer
  memAlloc.memoryTypeIndex =
      GetMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
  // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also
  // need to enable the appropriate flag during allocation
  VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
  if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
    allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
    allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
    memAlloc.pNext = &allocFlagsInfo;
  }
  VK_CHECK_RESULT(vkAllocateMemory(logicalDevice_, &memAlloc, nullptr, memory));

  // If a pointer to the buffer data has been passed, map the buffer and copy
  // over the data
  if (data != nullptr) {
    void *mapped;
    VK_CHECK_RESULT(vkMapMemory(logicalDevice_, *memory, 0, size, 0, &mapped));
    memcpy(mapped, data, size);
    // If host coherency hasn't been requested, do a manual flush to make writes
    // visible
    if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
      VkMappedMemoryRange mappedRange = initializers::MappedMemoryRange();
      mappedRange.memory = *memory;
      mappedRange.offset = 0;
      mappedRange.size = size;
      vkFlushMappedMemoryRanges(logicalDevice_, 1, &mappedRange);
    }
    vkUnmapMemory(logicalDevice_, *memory);
  }

  // Attach the memory to the buffer object
  VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice_, *buffer, *memory, 0));

  return VK_SUCCESS;
}

/**
 * Create a buffer on the device
 *
 * @param usageFlags Usage flag bit mask for the buffer (i.e. index, vertex,
 * uniform buffer)
 * @param memoryPropertyFlags Memory properties for this buffer (i.e. device
 * local, host visible, coherent)
 * @param buffer Pointer to a vk::Vulkan buffer object
 * @param size Size of the buffer in bytes
 * @param data Pointer to the data that should be copied to the buffer after
 * creation (optional, if not set, no data is copied over)
 *
 * @return VK_SUCCESS if buffer handle and memory have been created and
 * (optionally passed) data has been copied
 */
VkResult VulkanDevice::CreateBuffer(VkBufferUsageFlags usageFlags,
                                    VkMemoryPropertyFlags memoryPropertyFlags,
                                    VulkanBuffer *buffer, VkDeviceSize size,
                                    const void *data) {
  buffer->set_deivce(logicalDevice_);

  // Create the buffer handle
  VkBufferCreateInfo bufferCreateInfo =
      initializers::BufferCreateInfo(usageFlags, size);
  VK_CHECK_RESULT(vkCreateBuffer(logicalDevice_, &bufferCreateInfo, nullptr,
                                 &buffer->buffer_));

  // Create the memory backing up the buffer handle
  VkMemoryRequirements memReqs;
  VkMemoryAllocateInfo memAlloc = initializers::MemoryAllocateInfo();
  vkGetBufferMemoryRequirements(logicalDevice_, buffer->buffer_, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  // Find a memory type index that fits the properties of the buffer
  memAlloc.memoryTypeIndex =
      GetMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
  // If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also
  // need to enable the appropriate flag during allocation
  VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
  if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
    allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
    allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
    memAlloc.pNext = &allocFlagsInfo;
  }
  VK_CHECK_RESULT(
      vkAllocateMemory(logicalDevice_, &memAlloc, nullptr, &buffer->memory_));

  buffer->alignment_ = memReqs.alignment;
  buffer->size_ = size;
  buffer->usageFlags_ = usageFlags;
  buffer->memoryPropertyFlags_ = memoryPropertyFlags;

  // If a pointer to the buffer data has been passed, map the buffer and copy
  // over the data
  if (data != nullptr) {
    VK_CHECK_RESULT(buffer->Map());
    memcpy(buffer->mapped_, data, size);
    if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
      buffer->Flush();

    buffer->Unmap();
  }

  // Initialize a default descriptor that covers the whole buffer size
  buffer->SetupDescriptor();

  // Attach the memory to the buffer object
  return buffer->Bind();
}

void VulkanDevice::CopyBuffer(VulkanBuffer *src, VulkanBuffer *dst,
                              VkQueue queue, VkBufferCopy *copyRegion) {
  assert(dst->size_ <= src->size_);
  assert(src->buffer_);
  VkCommandBuffer copyCmd =
      CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
  VkBufferCopy bufferCopy{};
  if (copyRegion == nullptr) {
    bufferCopy.size = src->size_;
  } else {
    bufferCopy = *copyRegion;
  }

  vkCmdCopyBuffer(copyCmd, src->buffer_, dst->buffer_, 1, &bufferCopy);

  FlushCommandBuffer(copyCmd, queue);
}

VkCommandPool VulkanDevice::CreateCommandPool(
    uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags) {
  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
  cmdPoolInfo.flags = createFlags;
  VkCommandPool cmdPool;
  VK_CHECK_RESULT(
      vkCreateCommandPool(logicalDevice_, &cmdPoolInfo, nullptr, &cmdPool));
  return cmdPool;
}

VkCommandBuffer VulkanDevice::CreateCommandBuffer(VkCommandBufferLevel level,
                                                  VkCommandPool pool,
                                                  bool begin) {
  VkCommandBufferAllocateInfo cmdBufAllocateInfo =
      initializers::CommandBufferAllocateInfo(pool, level, 1);
  VkCommandBuffer cmdBuffer;
  VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice_, &cmdBufAllocateInfo,
                                           &cmdBuffer));
  // If requested, also start recording for the new command buffer
  if (begin) {
    VkCommandBufferBeginInfo cmdBufInfo =
        initializers::CommandBufferBeginInfo();
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
  }
  return cmdBuffer;
}

VkCommandBuffer VulkanDevice::CreateCommandBuffer(VkCommandBufferLevel level,
                                                  bool begin) {
  return CreateCommandBuffer(level, commandPool_, begin);
}

void VulkanDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer,
                                      VkQueue queue, VkCommandPool pool,
                                      bool free) {
  if (commandBuffer == VK_NULL_HANDLE) {
    return;
  }

  VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submitInfo = initializers::SubmitInfo();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  // Create fence to ensure that the command buffer has finished executing
  VkFenceCreateInfo fenceInfo = initializers::FenceCreateInfo(VK_FLAGS_NONE);
  VkFence fence;
  VK_CHECK_RESULT(vkCreateFence(logicalDevice_, &fenceInfo, nullptr, &fence));
  // Submit to the queue
  VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
  // Wait for the fence to signal that command buffer has finished executing
  VK_CHECK_RESULT(vkWaitForFences(logicalDevice_, 1, &fence, VK_TRUE,
                                  DEFAULT_FENCE_TIMEOUT));
  vkDestroyFence(logicalDevice_, fence, nullptr);
  if (free) {
    vkFreeCommandBuffers(logicalDevice_, pool, 1, &commandBuffer);
  }
}

void VulkanDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer,
                                      VkQueue queue, bool free) {
  return FlushCommandBuffer(commandBuffer, queue, commandPool_, free);
}

bool VulkanDevice::ExtensionSupported(std::string extension) {
  return (std::find(supportedExtensions_.begin(), supportedExtensions_.end(),
                    extension) != supportedExtensions_.end());
}

VkFormat VulkanDevice::GetSupportedDepthFormat(bool checkSamplingSupport) {
  // All depth formats may be optional, so we need to find a suitable depth
  // format to use
  std::vector<VkFormat> depthFormats = {
      VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM};
  for (auto &format : depthFormats) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice_, format,
                                        &formatProperties);
    // Format must support depth stencil attachment for optimal tiling
    if (formatProperties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      if (checkSamplingSupport) {
        if (!(formatProperties.optimalTilingFeatures &
              VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
          continue;
        }
      }
      return format;
    }
  }
  throw std::runtime_error("Could not find a matching depth format");
}
}  // namespace lvk