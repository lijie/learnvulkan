#include <array>
#include <iostream>

#include "base/vulkan_app.h"
#include "base/vulkan_buffer.h"
#include "base/vulkan_device.h"
#include "base/vulkan_initializers.h"
#include "base/vulkan_pipelinebuilder.h"
#include "base/vulkan_tools.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#include "glm/glm.hpp"

#define VERTEX_BUFFER_BIND_ID 0

namespace lvk {
using lvk::VulkanApp;

// Vertex layout for this example
struct Vertex {
  float pos[3];
  float uv[2];
  float normal[3];
};

class TriangleApp : public VulkanApp {
 private:
  struct {
    VkPipelineVertexInputStateCreateInfo inputState;
    std::vector<VkVertexInputBindingDescription> bindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
  } vertices;

  struct Texture {
    VkSampler sampler;
    VkImage image;
    VkImageLayout imageLayout;
    VkDeviceMemory deviceMemory;
    VkImageView view;
    uint32_t width, height;
    uint32_t mipLevels;
  } texture;

  lvk::VulkanBuffer uniformBufferVS;

  struct {
    glm::mat4 projection;
    glm::mat4 modelView;
    glm::vec4 viewPos;
    float lodBias = 0.0f;
  } uboVS;

  struct {
    VkPipeline solid;
  } pipelines;

  VkPipelineLayout pipelineLayout;
  VkDescriptorSet descriptorSet;
  VkDescriptorSetLayout descriptorSetLayout;

  lvk::VulkanBuffer vertexBuffer;
  lvk::VulkanBuffer indexBuffer;
  uint32_t indexCount;

  VkClearColorValue defaultClearColor = {{0.025f, 0.025f, 0.025f, 1.0f}};

  void LoadTexture();
  void GenerateQuad();
  void SetupVertexDescriptions();
  void PrepareUniformBuffers();
  void UpdateUniformBuffers();
  void SetupDescriptorSetLayout();
  void PreparePipelines();
  void SetupDescriptorPool();
  void SetupDescriptorSet();
  void BuildCommandBuffers();
  void Draw();

 public:
  void Render();
  virtual void Prepare() override;
};

void TriangleApp::GenerateQuad() {
  // Setup vertices for a single uv-mapped quad made from two triangles
  std::vector<Vertex> vertices = {{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
                                  {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
                                  {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                                  {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

  // Setup indices
  std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
  indexCount = static_cast<uint32_t>(indices.size());

  // Create buffers
  // For the sake of simplicity we won't stage the vertex data to the gpu memory
  // Vertex buffer
  VK_CHECK_RESULT(vulkanDevice->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                             &vertexBuffer, vertices.size() * sizeof(Vertex), vertices.data()));
  // Index buffer
  VK_CHECK_RESULT(vulkanDevice->CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                             &indexBuffer, indices.size() * sizeof(uint32_t), indices.data()));
}

void TriangleApp::SetupVertexDescriptions() {
  // Binding description
  vertices.bindingDescriptions.resize(1);
  vertices.bindingDescriptions[0] = lvk::initializers::VertexInputBindingDescription(
      VERTEX_BUFFER_BIND_ID, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX);

  // Attribute descriptions
  // Describes memory layout and shader positions
  vertices.attributeDescriptions.resize(3);
  // Location 0 : Position
  vertices.attributeDescriptions[0] = lvk::initializers::VertexInputAttributeDescription(
      VERTEX_BUFFER_BIND_ID, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos));
  // Location 1 : Texture coordinates
  vertices.attributeDescriptions[1] = lvk::initializers::VertexInputAttributeDescription(
      VERTEX_BUFFER_BIND_ID, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv));
  // Location 2 : Vertex normal
  vertices.attributeDescriptions[2] = lvk::initializers::VertexInputAttributeDescription(
      VERTEX_BUFFER_BIND_ID, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal));

  vertices.inputState = lvk::initializers::PipelineVertexInputStateCreateInfo();
  vertices.inputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertices.bindingDescriptions.size());
  vertices.inputState.pVertexBindingDescriptions = vertices.bindingDescriptions.data();
  vertices.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertices.attributeDescriptions.size());
  vertices.inputState.pVertexAttributeDescriptions = vertices.attributeDescriptions.data();
}

void TriangleApp::UpdateUniformBuffers() {
  uboVS.projection = glm::perspective(glm::radians(45.0f), float(width / height), 0.1f, 10.0f);

  auto view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  uboVS.modelView = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * view;
  // uboVS.viewPos = camera.viewPos;
  memcpy(uniformBufferVS.mapped(), &uboVS, sizeof(uboVS));
}

// Prepare and initialize uniform buffer containing shader uniforms
void TriangleApp::PrepareUniformBuffers() {
  // Vertex shader uniform buffer block
  VK_CHECK_RESULT(vulkanDevice->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                             &uniformBufferVS, sizeof(uboVS), &uboVS));
  VK_CHECK_RESULT(uniformBufferVS.Map());

  UpdateUniformBuffers();
}

void TriangleApp::SetupDescriptorSetLayout() {
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      // Binding 0 : Vertex shader uniform buffer
      lvk::initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
      // Binding 1 : Fragment shader image sampler
      lvk::initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                    VK_SHADER_STAGE_FRAGMENT_BIT, 1)};

  VkDescriptorSetLayoutCreateInfo descriptorLayout = lvk::initializers::DescriptorSetLayoutCreateInfo(
      setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));

  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

  VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
      lvk::initializers::PipelineLayoutCreateInfo(&descriptorSetLayout, 1);

  VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
}

void TriangleApp::PreparePipelines() {
  std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
  colorBlendAttachmentStates.push_back(initializers::PipelineColorBlendAttachmentState(0xf, VK_FALSE));

  // Load shaders
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  shaderStages.push_back(LoadShader(GetShadersPath() + "05-vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
  shaderStages.push_back(LoadShader(GetShadersPath() + "05-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));

  VulkanPipelineBuilder()
      .dynamicStates({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
      .primitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
      .polygonMode(VK_POLYGON_MODE_FILL)
      .vertexInputState(vertices.inputState)
      .cullMode(VK_CULL_MODE_NONE)
      .frontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
      .colorBlendAttachmentStates(colorBlendAttachmentStates)
      .depthWriteEnable(true)
      .depthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
      .rasterizationSamples(VK_SAMPLE_COUNT_1_BIT)
      .shaderStages(shaderStages)
      .build(device, pipelineCache, pipelineLayout, renderPass, &pipelines.solid, "05-GraphicPipline");
}

void TriangleApp::SetupDescriptorPool() {
  // Example uses one ubo and one image sampler
  std::vector<VkDescriptorPoolSize> poolSizes = {
      initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
      initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};

  VkDescriptorPoolCreateInfo descriptorPoolInfo =
      initializers::DescriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), 2);

  VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
}

void TriangleApp::SetupDescriptorSet() {
  VkDescriptorSetAllocateInfo allocInfo =
      initializers::DescriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);

  VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

  // Setup a descriptor image info for the current texture to be used as a
  // combined image sampler
  VkDescriptorImageInfo textureDescriptor;
  textureDescriptor.imageView = texture.view;           // The image's view (images are never directly accessed by the
                                                        // shader, but rather through views defining subresources)
  textureDescriptor.sampler = texture.sampler;          // The sampler (Telling the pipeline how to sample the
                                                        // texture, including repeat, border, etc.)
  textureDescriptor.imageLayout = texture.imageLayout;  // The current layout of the image (Note: Should
                                                        // always fit the actual use, e.g. shader read)

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      // Binding 0 : Vertex shader uniform buffer
      initializers::WriteDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                       &uniformBufferVS.descriptor_),
      // Binding 1 : Fragment shader texture sampler
      //	Fragment shader: layout (binding = 1) uniform sampler2D
      // samplerColor;
      initializers::WriteDescriptorSet(descriptorSet,
                                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // The descriptor set will
                                                                                   // use a combined image
                                                                                   // sampler (sampler and
                                                                                   // image could be split)
                                       1,                                          // Shader binding point 1
                                       &textureDescriptor)  // Pointer to the descriptor image for our
                                                            // texture
  };

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0,
                         NULL);
}

void TriangleApp::BuildCommandBuffers() {
  VkCommandBufferBeginInfo cmdBufInfo = initializers::CommandBufferBeginInfo();

  VkClearValue clearValues[2];
  clearValues[0].color = defaultClearColor;
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo renderPassBeginInfo = initializers::RenderPassBeginInfo();
  renderPassBeginInfo.renderPass = renderPass;
  renderPassBeginInfo.renderArea.offset.x = 0;
  renderPassBeginInfo.renderArea.offset.y = 0;
  renderPassBeginInfo.renderArea.extent.width = width;
  renderPassBeginInfo.renderArea.extent.height = height;
  renderPassBeginInfo.clearValueCount = 2;
  renderPassBeginInfo.pClearValues = clearValues;

  for (int32_t i = 0; i < drawCmdBuffers.size(); ++i) {
    // Set target frame buffer
    renderPassBeginInfo.framebuffer = frameBuffers[i];

    VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

    vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = initializers::Viewport((float)width, (float)height, 0.0f, 1.0f);
    vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

    VkRect2D scissor = initializers::Rect2D(width, height, 0, 0);
    vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

    vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0,
                            NULL);
    vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.solid);

    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &vertexBuffer.buffer_, offsets);
    vkCmdBindIndexBuffer(drawCmdBuffers[i], indexBuffer.buffer_, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(drawCmdBuffers[i], indexCount, 1, 0, 0, 0);

    // drawUI(drawCmdBuffers[i]);

    vkCmdEndRenderPass(drawCmdBuffers[i]);

    VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
  }
}

void TriangleApp::LoadTexture() {
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
  stbi_uc *pixels = stbi_load("../../assets/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  VkDeviceSize imageSize = texWidth * texHeight * 4;

#if defined(__ANDROID__)
#else
#endif

  VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

  // Get properties required for using and upload texture data from the ktx
  // texture object
  texture.width = texWidth;
  texture.height = texHeight;
  texture.mipLevels = 1;  // ktxTexture->numLevels;
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
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);
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
    VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer));

    // Get memory requirements for the staging buffer (alignment, memory type
    // bits)
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReqs);
    memAllocInfo.allocationSize = memReqs.size;
    // Get memory type index for a host visible buffer
    memAllocInfo.memoryTypeIndex = vulkanDevice->GetMemoryType(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &stagingMemory));
    VK_CHECK_RESULT(vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0));

    // Copy texture data into host local staging buffer
    uint8_t *data;
    VK_CHECK_RESULT(vkMapMemory(device, stagingMemory, 0, memReqs.size, 0, (void **)&data));
    memcpy(data, pixels, imageSize);
    vkUnmapMemory(device, stagingMemory);

    // Setup buffer copy regions for each mip level
    std::vector<VkBufferImageCopy> bufferCopyRegions;
    uint32_t offset = 0;

    for (uint32_t i = 0; i < texture.mipLevels; i++) {
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
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = texture.mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // Set initial layout of the image to undefined
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = {texture.width, texture.height, 1};
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &texture.image));

    vkGetImageMemoryRequirements(device, texture.image, &memReqs);
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex =
        vulkanDevice->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &texture.deviceMemory));
    VK_CHECK_RESULT(vkBindImageMemory(device, texture.image, texture.deviceMemory, 0));

    VkCommandBuffer copyCmd = vulkanDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    // Image memory barriers for the texture image

    // The sub resource range describes the regions of the image that will be
    // transitioned using the memory barriers below
    VkImageSubresourceRange subresourceRange = {};
    // Image only contains color data
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // Start at first mip level
    subresourceRange.baseMipLevel = 0;
    // We will transition on all mip levels
    subresourceRange.levelCount = texture.mipLevels;
    // The 2D texture only has one layer
    subresourceRange.layerCount = 1;

    // Transition the texture image layout to transfer target, so we can safely
    // copy our buffer data to it.
    VkImageMemoryBarrier imageMemoryBarrier = initializers::ImageMemoryBarrier();
    ;
    imageMemoryBarrier.image = texture.image;
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
    vkCmdCopyBufferToImage(copyCmd, stagingBuffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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
    texture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vulkanDevice->FlushCommandBuffer(copyCmd, queue, true);

    // Clean up staging resources
    vkFreeMemory(device, stagingMemory, nullptr);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
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
  sampler.maxLod = (useStaging) ? (float)texture.mipLevels : 0.0f;
  // Enable anisotropic filtering
  // This feature is optional, so we must check if it's supported on the device
  if (vulkanDevice->features().samplerAnisotropy) {
    // Use max. level of anisotropy for this example
    sampler.maxAnisotropy = vulkanDevice->properties().limits.maxSamplerAnisotropy;
    sampler.anisotropyEnable = VK_TRUE;
  } else {
    // The device does not support anisotropic filtering
    sampler.maxAnisotropy = 1.0;
    sampler.anisotropyEnable = VK_FALSE;
  }
  sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &texture.sampler));

  // Create image view
  // Textures are not directly accessed by the shaders and
  // are abstracted by image views containing additional
  // information and sub resource ranges
  VkImageViewCreateInfo view = initializers::ImageViewCreateInfo();
  view.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view.format = format;
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
  view.subresourceRange.levelCount = (useStaging) ? texture.mipLevels : 1;
  // The view will be based on the texture's image
  view.image = texture.image;
  VK_CHECK_RESULT(vkCreateImageView(device, &view, nullptr, &texture.view));
}

void TriangleApp::Prepare() {
  VulkanApp::Prepare();
  LoadTexture();
  GenerateQuad();
  SetupVertexDescriptions();
  PrepareUniformBuffers();
  SetupDescriptorSetLayout();
  PreparePipelines();
  SetupDescriptorPool();
  SetupDescriptorSet();
  BuildCommandBuffers();
  prepared = true;
}

void TriangleApp::Draw() {
  VulkanApp::PrepareFrame();

  // Command buffer to be submitted to the queue
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

  // Submit to queue
  VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  VulkanApp::SubmitFrame();
}

void TriangleApp::Render() {
  if (!prepared) return;
  Draw();
}
}  // namespace lvk

FILE *g_ic_file_cout_stream; FILE *g_ic_file_cin_stream;

// Success: true , Failure: false
bool InitConsole() 
{
    if (!AllocConsole()) { return false; }
    if (freopen_s(&g_ic_file_cout_stream, "CONOUT$", "w", stdout) != 0) { return false; } // For std::cout 
    if (freopen_s(&g_ic_file_cin_stream, "CONIN$", "w+", stdin) != 0) { return false; } // For std::cin
    return true;
}

using lvk::TriangleApp;
using lvk::VulkanApp;

static VulkanApp *vulkanApp{nullptr};
// Windows entry point
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  if (vulkanApp != NULL) {
    vulkanApp->HandleMessages(hWnd, uMsg, wParam, lParam);
  }
  return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
  InitConsole();
  for (int32_t i = 0; i < __argc; i++) {
    VulkanApp::args.push_back(__argv[i]);
  };
  vulkanApp = new TriangleApp();
  vulkanApp->InitVulkan();
  vulkanApp->SetupWindow(hInstance, WndProc);
  vulkanApp->Prepare();
  vulkanApp->RenderLoop();
  delete (vulkanApp);
  return 0;
}
