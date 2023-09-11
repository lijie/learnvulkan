#include "vulkan_texture.h"

#include "vulkan_device.h"
#include "vulkan_initializers.h"
#include "vulkan_tools.h"

// #define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace lvk {

void VulkanTexture::LoadTexture() {
#if 0
  // We use the Khronos texture format
  // (https://www.khronos.org/opengles/sdk/tools/KTX/file_format_spec/)
  std::string filename = getAssetPath() + "textures/metalplate01_rgba.ktx";
  // Texture data contains 4 channels (RGBA) with unnormalized 8-bit values,
  // this is the most commonly supported format
  VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

  ktxResult result;
  ktxTexture *ktxTexture;
#endif
  int texWidth, texHeight, texChannels;
  stbi_uc *pixels = stbi_load(path_.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  VkDeviceSize imageSize = texWidth * texHeight * 4;

#if defined(__ANDROID__)
#else
#endif

  // VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

  // Get properties required for using and upload texture data from the ktx
  // texture object
  width_ = texWidth;
  height_ = texHeight;
  mipLevels_ = 1;  // ktxTexture->numLevels;
  // ktx_uint8_t *ktxTextureData = ktxTexture_GetData(ktxTexture);
  // ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

  // We prefer using staging to copy the texture data to a device local optimal
  // image
  VkBool32 useStaging = true;

  // Only use linear tiling if forced
  bool forceLinearTiling = false;
  if (forceLinearTiling) {
    // Don't use linear if format is not supported for (linear) shader sampling
    // Get device properties for the requested texture format
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(device_->physicalDevice(), format_, &formatProperties);
    useStaging = !(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
  }

  VkMemoryAllocateInfo memAllocInfo = initializers::MemoryAllocateInfo();
  VkMemoryRequirements memReqs = {};

  if (useStaging) {
    // Copy data to an optimal tiled image
    // This loads the texture data into a host local buffer that is copied to
    // the optimal tiled image on the device

    // Create a host-visible staging buffer that contains the raw image data
    // This buffer will be the data source for copying texture data to the
    // optimal tiled image on the device
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    VkBufferCreateInfo bufferCreateInfo = initializers::BufferCreateInfo();
    bufferCreateInfo.size = imageSize;
    // This buffer is used as a transfer source for the buffer copy
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK_RESULT(vkCreateBuffer(device_->device(), &bufferCreateInfo, nullptr, &stagingBuffer));

    // Get memory requirements for the staging buffer (alignment, memory type
    // bits)
    vkGetBufferMemoryRequirements(device_->device(), stagingBuffer, &memReqs);
    memAllocInfo.allocationSize = memReqs.size;
    // Get memory type index for a host visible buffer
    memAllocInfo.memoryTypeIndex = device_->GetMemoryType(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device_->device(), &memAllocInfo, nullptr, &stagingMemory));
    VK_CHECK_RESULT(vkBindBufferMemory(device_->device(), stagingBuffer, stagingMemory, 0));

    // Copy texture data into host local staging buffer
    uint8_t *data;
    VK_CHECK_RESULT(vkMapMemory(device_->device(), stagingMemory, 0, memReqs.size, 0, (void **)&data));
    memcpy(data, pixels, imageSize);
    vkUnmapMemory(device_->device(), stagingMemory);

    // Setup buffer copy regions for each mip level
    std::vector<VkBufferImageCopy> bufferCopyRegions;
    uint32_t offset = 0;

    for (uint32_t i = 0; i < mipLevels_; i++) {
      // Calculate offset into staging buffer for the current mip level
      // Setup a buffer image copy structure for the current mip level
      VkBufferImageCopy bufferCopyRegion = {};
      bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      bufferCopyRegion.imageSubresource.mipLevel = i;
      bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
      bufferCopyRegion.imageSubresource.layerCount = 1;
      bufferCopyRegion.imageExtent.width = texWidth >> i;
      bufferCopyRegion.imageExtent.height = texHeight >> i;
      bufferCopyRegion.imageExtent.depth = 1;
      bufferCopyRegion.bufferOffset = offset;
      bufferCopyRegions.push_back(bufferCopyRegion);
    }

    // Create optimal tiled target image on the device
    VkImageCreateInfo imageCreateInfo = initializers::ImageCreateInfo();
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format_;
    imageCreateInfo.mipLevels = mipLevels_;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // Set initial layout of the image to undefined
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = {width_, height_, 1};
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    VK_CHECK_RESULT(vkCreateImage(device_->device(), &imageCreateInfo, nullptr, &image_));

    vkGetImageMemoryRequirements(device_->device(), image_, &memReqs);
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = device_->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device_->device(), &memAllocInfo, nullptr, &deviceMemory_));
    VK_CHECK_RESULT(vkBindImageMemory(device_->device(), image_, deviceMemory_, 0));

    VkCommandBuffer copyCmd = device_->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    // Image memory barriers for the texture image

    // The sub resource range describes the regions of the image that will be
    // transitioned using the memory barriers below
    VkImageSubresourceRange subresourceRange = {};
    // Image only contains color data
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // Start at first mip level
    subresourceRange.baseMipLevel = 0;
    // We will transition on all mip levels
    subresourceRange.levelCount = mipLevels_;
    // The 2D texture only has one layer
    subresourceRange.layerCount = 1;

    // Transition the texture image layout to transfer target, so we can safely
    // copy our buffer data to it.
    VkImageMemoryBarrier imageMemoryBarrier = initializers::ImageMemoryBarrier();
    ;
    imageMemoryBarrier.image = image_;
    imageMemoryBarrier.subresourceRange = subresourceRange;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    // Insert a memory dependency at the proper pipeline stages that will
    // execute the image layout transition Source pipeline stage is host
    // write/read execution (VK_PIPELINE_STAGE_HOST_BIT) Destination pipeline
    // stage is copy command execution (VK_PIPELINE_STAGE_TRANSFER_BIT)
    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                         1, &imageMemoryBarrier);

    // Copy mip levels from staging buffer
    vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());

    // Once the data has been uploaded we transfer to the texture image to the
    // shader read layout, so it can be sampled from
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Insert a memory dependency at the proper pipeline stages that will
    // execute the image layout transition Source pipeline stage is copy command
    // execution (VK_PIPELINE_STAGE_TRANSFER_BIT) Destination pipeline stage
    // fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &imageMemoryBarrier);

    // Store current layout for later reuse
    imageLayout_ = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    device_->FlushCommandBuffer(copyCmd, queue_, true);

    // Clean up staging resources
    vkFreeMemory(device_->device(), stagingMemory, nullptr);
    vkDestroyBuffer(device_->device(), stagingBuffer, nullptr);
  } else {
  }

  // Create a texture sampler
  // In Vulkan textures are accessed by samplers
  // This separates all the sampling information from the texture data. This
  // means you could have multiple sampler objects for the same texture with
  // different settings Note: Similar to the samplers available with OpenGL 3.3
  VkSamplerCreateInfo sampler = initializers::SamplerCreateInfo();
  sampler.magFilter = VK_FILTER_LINEAR;
  sampler.minFilter = VK_FILTER_LINEAR;
  sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  sampler.mipLodBias = 0.0f;
  sampler.compareOp = VK_COMPARE_OP_NEVER;
  sampler.minLod = 0.0f;
  // Set max level-of-detail to mip level count of the texture
  sampler.maxLod = (useStaging) ? (float)mipLevels_ : 0.0f;
  // Enable anisotropic filtering
  // This feature is optional, so we must check if it's supported on the device
  if (device_->features().samplerAnisotropy) {
    // Use max. level of anisotropy for this example
    sampler.maxAnisotropy = device_->properties().limits.maxSamplerAnisotropy;
    sampler.anisotropyEnable = VK_FALSE;
  } else {
    // The device does not support anisotropic filtering
    sampler.maxAnisotropy = 1.0;
    sampler.anisotropyEnable = VK_FALSE;
  }
  sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  VK_CHECK_RESULT(vkCreateSampler(device_->device(), &sampler, nullptr, &sampler_));

  // Create image view
  // Textures are not directly accessed by the shaders and
  // are abstracted by image views containing additional
  // information and sub resource ranges
  VkImageViewCreateInfo view = initializers::ImageViewCreateInfo();
  view.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view.format = format_;
  // The subresource range describes the set of mip levels (and array layers)
  // that can be accessed through this image view It's possible to create
  // multiple image views for a single image referring to different (and/or
  // overlapping) ranges of the image
  view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view.subresourceRange.baseMipLevel = 0;
  view.subresourceRange.baseArrayLayer = 0;
  view.subresourceRange.layerCount = 1;
  // Linear tiling usually won't support mip maps
  // Only set mip map count if optimal tiling is used
  view.subresourceRange.levelCount = (useStaging) ? mipLevels_ : 1;
  // The view will be based on the texture's image
  view.image = image_;
  VK_CHECK_RESULT(vkCreateImageView(device_->device(), &view, nullptr, &view_));
}

VkDescriptorImageInfo VulkanTexture::GetDescriptorImageInfo() {
  // Setup a descriptor image info for the current texture to be used as a
  // combined image sampler
  VkDescriptorImageInfo textureDescriptor;
  textureDescriptor.imageView = view_;           // The image's view (images are never directly accessed by the
                                                 // shader, but rather through views defining subresources)
  textureDescriptor.sampler = sampler_;          // The sampler (Telling the pipeline how to sample the
                                                 // texture, including repeat, border, etc.)
  textureDescriptor.imageLayout = imageLayout_;  // The current layout of the image (Note: Should
  // always fit the actual use, e.g. shader read)
  return textureDescriptor;
}
}  // namespace lvk