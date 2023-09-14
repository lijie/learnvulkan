#include "vulkan_context.h"

#include <corecrt_malloc.h>

#include <iostream>
#include <vector>

#include "primitives.h"
#include "scene.h"
#include "vertex_data.h"
#include "vulkan/vulkan_core.h"
#include "vulkan_debug.h"
#include "vulkan_device.h"
#include "vulkan_initializers.h"
#include "vulkan_pipelinebuilder.h"
#include "vulkan_tools.h"

#define VERTEX_BUFFER_BIND_ID 0

namespace lvk {

static VkClearColorValue defaultClearColor = {{0.025f, 0.025f, 0.025f, 1.0f}};

VulkanContext::VulkanContext() { VK_CHECK_RESULT(CreateInstance(true)); }

VulkanContext::~VulkanContext() {
  if (uniformBuffers_.model) {
    free(uniformBuffers_.model);
  }
  for (const auto& tex : vkTextureList) {
    delete tex;
  }
}

void VulkanContext::InitWithOptions(const VulkanContextOptions& options, VkPhysicalDevice phy_device) {
  options_ = options;

  vkGetDeviceQueue(device_->device(), device_->queueFamilyIndices_.graphics, 0, &queue_);

  swapChain_.Connect(instance_, phy_device, device_->device());

  // Create synchronization objects
  VkSemaphoreCreateInfo semaphoreCreateInfo = initializers::SemaphoreCreateInfo();
  // Create a semaphore used to synchronize image presentation
  // Ensures that the image is displayed before we start submitting new commands
  // to the queue
  VK_CHECK_RESULT(vkCreateSemaphore(device_->device(), &semaphoreCreateInfo, nullptr, &semaphores_.presentComplete));
  // Create a semaphore used to synchronize command submission
  // Ensures that the image is not presented until all commands have been
  // submitted and executed
  VK_CHECK_RESULT(vkCreateSemaphore(device_->device(), &semaphoreCreateInfo, nullptr, &semaphores_.renderComplete));

  // Set up submit info structure
  // Semaphores will stay the same during application lifetime
  // Command buffer submission info is set by each example
  submitInfo_ = initializers::SubmitInfo();
  submitInfo_.pWaitDstStageMask = &submitPipelineStages_;
  submitInfo_.waitSemaphoreCount = 1;
  submitInfo_.pWaitSemaphores = &semaphores_.presentComplete;
  submitInfo_.signalSemaphoreCount = 1;
  submitInfo_.pSignalSemaphores = &semaphores_.renderComplete;
}

void VulkanContext::CreateVulkanScene(Scene* scene, VulkanDevice* device) {
  Prepare();

  vkNodeList.resize(scene->GetNodeCount());
  vkMeshList.resize(scene->GetNodeCount());
  for (int i = 0; i < scene->GetNodeCount(); i++) {
    const auto& node = scene->GetNode(i);
    auto& vknode = vkNodeList[i];
    vkMeshList[i].CreateBuffer(scene->GetResourceMesh(i), device);
    vkMeshList[i].indexCount = scene->GetResourceMesh(i)->indices.size();

    if (node->materialParamters.textureList.size() > 0) {
      auto texture_handle = node->materialParamters.textureList[0];
      auto texture = new VulkanTexture(device, scene->GetResourceTexture(texture_handle)->path, queue_);
      texture->LoadTexture();
      vkTextureList.push_back(texture);
      vknode.vkTexture = vkTextureList.back();
    }
    vknode.vkMesh = &vkMeshList[i];

    LoadMaterial(&vknode, scene->GetResourceMaterial(node->material), device);
    vknode.pipelineHandle = FindOrCreatePipeline();
  }

  PrepareUniformBuffers(scene, device);
  SetupDescriptorSetLayout(device);
  SetupDescriptorPool(device);
  BuildPipelines();
  SetupDescriptorSet();
}

void VulkanContext::PrepareUniformBuffers(Scene* scene, VulkanDevice* device) {
  size_t bufferSize = scene->GetNodeCount() * sizeof(_UBOMesh);

  VK_CHECK_RESULT(device->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &uniformBuffers_.dynamic, bufferSize));

  VK_CHECK_RESULT(device->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &uniformBuffers_.view, sizeof(CameraMatrix)));

  // save all model matrix
  uniformBuffers_.model = reinterpret_cast<_UBOMesh*>(malloc(bufferSize));
  uniformBuffers_.dynamicBufferSize = bufferSize;
  uniformBuffers_.dynamicAlignment = sizeof(_UBOMesh);

  UpdateUniformBuffers(scene);
}

void VulkanContext::UpdateUniformBuffers(Scene* scene) {
  const auto& camera_matrix = scene->GetCameraMatrix();
  // update model matrix
  scene->ForEachNode([this, camera_matrix](Node* n, int idx) {
    auto& ubo = uniformBuffers_.model[idx];
    ubo.modelView = camera_matrix.view * n->localMatrix();
    ubo.projection = camera_matrix.proj;
  });

  // update to gpu
  uniformBuffers_.dynamic.Update(reinterpret_cast<const uint8_t*>(uniformBuffers_.model),
                                 uniformBuffers_.dynamicBufferSize);
}

void VulkanContext::SetupDescriptorSetLayout(VulkanDevice* device) {
  std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
      initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
      initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT,
                                               1),
      initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
                                               2)};

  VkDescriptorSetLayoutCreateInfo descriptor_layout = initializers::DescriptorSetLayoutCreateInfo(
      set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->device(), &descriptor_layout, nullptr, &descriptorSetLayout_));

  VkPipelineLayoutCreateInfo pipeline_layout_create_info =
      initializers::PipelineLayoutCreateInfo(&descriptorSetLayout_, 1);

  VK_CHECK_RESULT(vkCreatePipelineLayout(device->device(), &pipeline_layout_create_info, nullptr, &pipelineLayout_));
}

void VulkanContext::SetupDescriptorPool(VulkanDevice* device) {
  // Example uses one ubo and one image sampler
  std::vector<VkDescriptorPoolSize> pool_sizes = {
      initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
      initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1),
      initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};

  VkDescriptorPoolCreateInfo descriptor_pool_create_info =
      initializers::DescriptorPoolCreateInfo(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), 2);

  VK_CHECK_RESULT(vkCreateDescriptorPool(device->device(), &descriptor_pool_create_info, nullptr, &descriptorPool_));
}

const VkPipelineVertexInputStateCreateInfo& VulkanContext::BuildVertexInputState() {
  vertexInputState_.bindingDescriptions[0] = lvk::initializers::VertexInputBindingDescription(
      VERTEX_BUFFER_BIND_ID, sizeof(VertexLayout), VK_VERTEX_INPUT_RATE_VERTEX);

  // Attribute descriptions
  // Describes memory layout and shader positions
  // Location 0 : Position
  vertexInputState_.attributeDescriptions[0] = lvk::initializers::VertexInputAttributeDescription(
      VERTEX_BUFFER_BIND_ID, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexLayout, position));
  // Location 1 : Vertex normal
  vertexInputState_.attributeDescriptions[1] = lvk::initializers::VertexInputAttributeDescription(
      VERTEX_BUFFER_BIND_ID, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexLayout, normal));
  // Location 2 : Texture coordinates
  vertexInputState_.attributeDescriptions[2] = lvk::initializers::VertexInputAttributeDescription(
      VERTEX_BUFFER_BIND_ID, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexLayout, uv));

  vertexInputState_.inputState = lvk::initializers::PipelineVertexInputStateCreateInfo();
  vertexInputState_.inputState.vertexBindingDescriptionCount =
      static_cast<uint32_t>(vertexInputState_.bindingDescriptions.size());
  vertexInputState_.inputState.pVertexBindingDescriptions = vertexInputState_.bindingDescriptions.data();
  vertexInputState_.inputState.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(vertexInputState_.attributeDescriptions.size());
  vertexInputState_.inputState.pVertexAttributeDescriptions = vertexInputState_.attributeDescriptions.data();
  return vertexInputState_.inputState;
}

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
      std::cout << "supported extensions:" << std::endl;
      for (VkExtensionProperties extension : extensions) {
        supportedInstanceExtensions.push_back(extension.extensionName);
        std::cout << "\t" << extension.extensionName << std::endl;
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
    std::cout << "InstanceLayer:" << std::endl;
    for (VkLayerProperties layer : instanceLayerProperties) {
      if (strcmp(layer.layerName, validationLayerName) == 0) {
        validationLayerPresent = true;
        break;
      }
      std::cout << "\t" << layer.layerName << std::endl;
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

VkDescriptorBufferInfo VulkanContext::CreateDescriptor(VulkanBuffer* buffer, VkDeviceSize size, VkDeviceSize offset) {
  VkDescriptorBufferInfo descriptor{};
  descriptor.buffer = buffer->buffer();
  descriptor.range = size;
  descriptor.offset = offset;
  return descriptor;
}

VkPipelineShaderStageCreateInfo VulkanContext::LoadShader(std::string fileName, VkShaderStageFlagBits stage,
                                                          VulkanDevice* device) {
  VkPipelineShaderStageCreateInfo shaderStage = {};
  shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStage.stage = stage;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
  shaderStage.module = vks::tools::loadShader(androidApp->activity->assetManager, fileName.c_str(), device);
#else
  shaderStage.module = tools::LoadShader(fileName.c_str(), device->device());
#endif
  shaderStage.pName = "main";
  assert(shaderStage.module != VK_NULL_HANDLE);
  // shaderModules.push_back(shaderStage.module);
  return shaderStage;
}

bool VulkanContext::LoadMaterial(VulkanNode* vkNode, const Material* mat, VulkanDevice* device) {
  vkNode->shaderStages.push_back(LoadShader(mat->vertShaderPath, VK_SHADER_STAGE_VERTEX_BIT, device));
  vkNode->shaderStages.push_back(LoadShader(mat->fragShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT, device));
  return true;
}

void VulkanContext::SetupRenderPass() {
  VkFormat color_format = swapChain_.colorFormat_;
  VkFormat depth_format = options_.depthFormat;

  std::array<VkAttachmentDescription, 2> attachments = {};
  // Color attachment
  attachments[0].format = color_format;  // swapChain.colorFormat_;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  // Depth attachment
  attachments[1].format = depth_format;  // depthFormat;
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
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[0].dstAccessMask =
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
  dependencies[0].dependencyFlags = 0;

  dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].dstSubpass = 0;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].srcAccessMask = 0;
  dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  dependencies[1].dependencyFlags = 0;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDescription;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  VK_CHECK_RESULT(vkCreateRenderPass(device_->device(), &renderPassInfo, nullptr, &renderPass_));
}

void VulkanContext::CreatePipelineCache() {
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  VK_CHECK_RESULT(vkCreatePipelineCache(device_->device(), &pipelineCacheCreateInfo, nullptr, &pipelineCache_));
}

void VulkanContext::BuildPipelines() {
  // todo
  CreatePipelineCache();
  // todo
  pipelineList.resize(1);

  std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
  colorBlendAttachmentStates.push_back(initializers::PipelineColorBlendAttachmentState(0xf, VK_FALSE));

  VulkanPipelineBuilder()
      .dynamicStates({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
      .primitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
      .polygonMode(VK_POLYGON_MODE_FILL)
      .vertexInputState(BuildVertexInputState())
      .cullMode(VK_CULL_MODE_NONE)
      .frontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
      .colorBlendAttachmentStates(colorBlendAttachmentStates)
      .depthWriteEnable(true)
      .depthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
      .rasterizationSamples(VK_SAMPLE_COUNT_1_BIT)
      .shaderStages(GetVkNode(0)->shaderStages)
      .build(device_->device(), pipelineCache_, PipelineLayout(), renderPass_, &pipelineList[0], "05-GraphicPipline");
}

void VulkanContext::SetupDescriptorSet() {
  // todo:
  descriptorSetList.resize(1);

  VkDescriptorSetAllocateInfo allocInfo =
      initializers::DescriptorSetAllocateInfo(descriptorPool_, &descriptorSetLayout_, 1);

  VK_CHECK_RESULT(vkAllocateDescriptorSets(device_->device(), &allocInfo, &descriptorSetList[0]));

  // Setup a descriptor image info for the current texture to be used as a
  // combined image sampler
  VkDescriptorImageInfo textureDescriptor =
      GetVkNode(0)->vkTexture->GetDescriptorImageInfo();  // texture_->GetDescriptorImageInfo();

  auto descriptor = CreateDescriptor2();

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
    // Binding 0 : Vertex shader uniform buffer
    initializers::WriteDescriptorSet(descriptorSetList[0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, &descriptor),
  // &uniformBufferVS.descriptor_),
// Binding 1 : Fragment shader texture sampler
//	Fragment shader: layout (binding = 1) uniform sampler2D
// samplerColor;
#if 1
    initializers::WriteDescriptorSet(descriptorSetList[0],
                                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // The descriptor set will
                                                                                 // use a combined image
                                                                                 // sampler (sampler and
                                                                                 // image could be split)
                                     2,                                          // Shader binding point 1
                                     &textureDescriptor)  // Pointer to the descriptor image for our
                                                          // texture
#endif
  };

  vkUpdateDescriptorSets(device_->device(), static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, NULL);
}

void VulkanContext::InitSwapchain() { swapChain_.InitSurface(NULL, options_.window); }

void VulkanContext::SetupSwapchain() { swapChain_.Create(&width, &height, false, false); }

void VulkanContext::SetupDepthStencil() {
  VkImageCreateInfo imageCI{};
  imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCI.imageType = VK_IMAGE_TYPE_2D;
  imageCI.format = options_.depthFormat;
  imageCI.extent = {width, height, 1};
  imageCI.mipLevels = 1;
  imageCI.arrayLayers = 1;
  imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VK_CHECK_RESULT(vkCreateImage(device_->device(), &imageCI, nullptr, &depthStencil_.image));
  VkMemoryRequirements memReqs{};
  vkGetImageMemoryRequirements(device_->device(), depthStencil_.image, &memReqs);

  VkMemoryAllocateInfo memAllloc{};
  memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memAllloc.allocationSize = memReqs.size;
  memAllloc.memoryTypeIndex = device_->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(vkAllocateMemory(device_->device(), &memAllloc, nullptr, &depthStencil_.mem));
  VK_CHECK_RESULT(vkBindImageMemory(device_->device(), depthStencil_.image, depthStencil_.mem, 0));

  VkImageViewCreateInfo imageViewCI{};
  imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewCI.image = depthStencil_.image;
  imageViewCI.format = options_.depthFormat;
  imageViewCI.subresourceRange.baseMipLevel = 0;
  imageViewCI.subresourceRange.levelCount = 1;
  imageViewCI.subresourceRange.baseArrayLayer = 0;
  imageViewCI.subresourceRange.layerCount = 1;
  imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  // Stencil aspect should only be set on depth + stencil formats
  // (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
  if (options_.depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
    imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }
  VK_CHECK_RESULT(vkCreateImageView(device_->device(), &imageViewCI, nullptr, &depthStencil_.view));
}

void VulkanContext::SetupFrameBuffer() {
  VkImageView attachments[2];

  // Depth/Stencil attachment is the same for all frame buffers
  attachments[1] = depthStencil_.view;

  VkFramebufferCreateInfo frameBufferCreateInfo = {};
  frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  frameBufferCreateInfo.pNext = NULL;
  frameBufferCreateInfo.renderPass = renderPass();  // renderPass;
  frameBufferCreateInfo.attachmentCount = 2;
  frameBufferCreateInfo.pAttachments = attachments;
  frameBufferCreateInfo.width = width;
  frameBufferCreateInfo.height = height;
  frameBufferCreateInfo.layers = 1;

  // Create frame buffers for every swap chain image
  frameBuffers_.resize(swapChain_.imageCount_);
  for (uint32_t i = 0; i < frameBuffers_.size(); i++) {
    attachments[0] = swapChain_.buffers_[i].view;
    VK_CHECK_RESULT(vkCreateFramebuffer(device_->device(), &frameBufferCreateInfo, nullptr, &frameBuffers_[i]));
  }
}

void VulkanContext::CreateCommandPool() {
  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.queueFamilyIndex = swapChain_.queueNodeIndex();
  cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VK_CHECK_RESULT(vkCreateCommandPool(device_->device(), &cmdPoolInfo, nullptr, &cmdPool_));
}

void VulkanContext::CreateCommandBuffers() {
  // Create one command buffer for each swap chain image and reuse for rendering
  drawCmdBuffers_.resize(swapChain_.imageCount());

  VkCommandBufferAllocateInfo cmdBufAllocateInfo = initializers::CommandBufferAllocateInfo(
      cmdPool_, VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(drawCmdBuffers_.size()));

  VK_CHECK_RESULT(vkAllocateCommandBuffers(device_->device(), &cmdBufAllocateInfo, drawCmdBuffers_.data()));
}

void VulkanContext::BuildCommandBuffers(Scene* scene) {
  VkCommandBufferBeginInfo cmdBufInfo = initializers::CommandBufferBeginInfo();

  VkClearValue clearValues[2];
  clearValues[0].color = defaultClearColor;
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo renderPassBeginInfo = initializers::RenderPassBeginInfo();
  renderPassBeginInfo.renderPass = renderPass();  // renderPass;
  renderPassBeginInfo.renderArea.offset.x = 0;
  renderPassBeginInfo.renderArea.offset.y = 0;
  renderPassBeginInfo.renderArea.extent.width = width;
  renderPassBeginInfo.renderArea.extent.height = height;
  renderPassBeginInfo.clearValueCount = 2;
  renderPassBeginInfo.pClearValues = clearValues;

  for (int32_t i = 0; i < drawCmdBuffers_.size(); ++i) {
    const auto& vkNode = GetVkNode(0);
    // Set target frame buffer
    renderPassBeginInfo.framebuffer = frameBuffers_[i];

    VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers_[i], &cmdBufInfo));

    vkCmdBeginRenderPass(drawCmdBuffers_[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = initializers::Viewport((float)width, (float)height, 0.0f, 1.0f);
    vkCmdSetViewport(drawCmdBuffers_[i], 0, 1, &viewport);

    VkRect2D scissor = initializers::Rect2D(width, height, 0, 0);
    vkCmdSetScissor(drawCmdBuffers_[i], 0, 1, &scissor);

    uint32_t dynamic_offset = 0;
    vkCmdBindDescriptorSets(drawCmdBuffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout(), 0, 1,
                            GetDescriptorSetP(vkNode->descriptorSetHandle), 1, &dynamic_offset);
    vkCmdBindPipeline(drawCmdBuffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, GetPipeline(vkNode->pipelineHandle));

    VkDeviceSize offsets[1] = {0};
    auto vkvb = vkNode->vkMesh->vertexBuffer->buffer();
    auto vkib = vkNode->vkMesh->indexBuffer->buffer();
    vkCmdBindVertexBuffers(drawCmdBuffers_[i], VERTEX_BUFFER_BIND_ID, 1, &vkvb, offsets);
    vkCmdBindIndexBuffer(drawCmdBuffers_[i], vkib, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(drawCmdBuffers_[i], vkNode->vkMesh->indexCount, 1, 0, 0, 0);

    // drawUI(drawCmdBuffers[i]);

    vkCmdEndRenderPass(drawCmdBuffers_[i]);

    VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers_[i]));
  }
}

void VulkanContext::CreateSynchronizationPrimitives() {
  // Wait fences to sync command buffer access
  VkFenceCreateInfo fenceCreateInfo = initializers::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
  waitFences_.resize(drawCmdBuffers_.size());
  for (auto& fence : waitFences_) {
    VK_CHECK_RESULT(vkCreateFence(device_->device(), &fenceCreateInfo, nullptr, &fence));
  }
}

void VulkanContext::PrepareFrame() {
  // Acquire the next image from the swap chain
  VkResult result = swapChain_.AcquireNextImage(semaphores_.presentComplete, &currentBuffer_);
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

void VulkanContext::SubmitFrame() {
  VkResult result = swapChain_.QueuePresent(queue_, currentBuffer_, semaphores_.renderComplete);
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
  VK_CHECK_RESULT(vkQueueWaitIdle(queue_));
}

void VulkanContext::Draw() {
  PrepareFrame();

  // Command buffer to be submitted to the queue
  submitInfo_.commandBufferCount = 1;
  submitInfo_.pCommandBuffers = &drawCmdBuffers_[currentBuffer_];

  // Submit to queue
  VK_CHECK_RESULT(vkQueueSubmit(queue_, 1, &submitInfo_, VK_NULL_HANDLE));

  SubmitFrame();
}

void VulkanContext::Prepare() {
  InitSwapchain();
  CreateCommandPool();
  SetupSwapchain();
  CreateCommandBuffers();
  CreateSynchronizationPrimitives();
  SetupDepthStencil();
  SetupRenderPass();
  SetupFrameBuffer();
}
}  // namespace lvk