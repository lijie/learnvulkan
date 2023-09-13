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
#include "vulkan_tools.h"
#include "vulkan_pipelinebuilder.h"

#define VERTEX_BUFFER_BIND_ID 0

namespace lvk {

VulkanContext::VulkanContext() { VK_CHECK_RESULT(CreateInstance(true)); }

VulkanContext::~VulkanContext() {
  if (uniformBuffers_.model) {
    free(uniformBuffers_.model);
  }
  for (const auto& tex : vkTextureList) {
    delete tex;
  }
}

void VulkanContext::CreateVulkanScene(Scene* scene, VulkanDevice* device) {
  VkQueue queue;
  vkGetDeviceQueue(device->device(), device->queueFamilyIndices_.graphics, 0, &queue);

  vkNodeList.resize(scene->GetNodeCount());
  vkMeshList.resize(scene->GetNodeCount());
  for (int i = 0; i < scene->GetNodeCount(); i++) {
    const auto& node = scene->GetNode(i);
    auto& vknode = vkNodeList[i];
    vkMeshList[i].CreateBuffer(scene->GetResourceMesh(i), device);
    vkMeshList[i].indexCount = scene->GetResourceMesh(i)->indices.size();

    if (node->materialParamters.textureList.size() > 0) {
      auto texture_handle = node->materialParamters.textureList[0];
      auto texture = new VulkanTexture(device, scene->GetResourceTexture(texture_handle)->path, queue);
      texture->LoadTexture();
      vkTextureList.push_back(texture);
      vknode.vkTexture = vkTextureList.back();
    }
    vknode.vkMesh = &vkMeshList[i];

    LoadMaterial(&vknode, scene->GetResourceMaterial(node->material), device);
  }

  PrepareUniformBuffers(scene, device);
  SetupDescriptorSetLayout(device);
  SetupDescriptorPool(device);
  BuildPipelines();
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

void VulkanContext::SetupDescriptorSet(VulkanDevice* device) {
  VkDescriptorSetAllocateInfo alloc_info =
      initializers::DescriptorSetAllocateInfo(descriptorPool_, &descriptorSetLayout_, 1);

  VK_CHECK_RESULT(vkAllocateDescriptorSets(device->device(), &alloc_info, &descriptorSet_));

  VkDescriptorBufferInfo view_buffer_descriptor = CreateDescriptor(&uniformBuffers_.view);

  // Pass the  actual dynamic alignment as the descriptor's size
  VkDescriptorBufferInfo dynamic_buffer_descriptor =
      CreateDescriptor(&uniformBuffers_.dynamic, uniformBuffers_.dynamicAlignment);

  std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
      // Binding 0 : Projection/View matrix uniform buffer
      initializers::WriteDescriptorSet(descriptorSet_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &view_buffer_descriptor),
      // Binding 1 : Instance matrix as dynamic uniform buffer
      initializers::WriteDescriptorSet(descriptorSet_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1,
                                       &dynamic_buffer_descriptor),
  };

  vkUpdateDescriptorSets(device->device(), static_cast<uint32_t>(write_descriptor_sets.size()),
                         write_descriptor_sets.data(), 0, NULL);
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

void VulkanContext::SetupRenderPass(VulkanDevice *device, VkFormat color_format, VkFormat depth_format) {
  std::array<VkAttachmentDescription, 2> attachments = {};
  // Color attachment
  attachments[0].format = color_format; //swapChain.colorFormat_;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  // Depth attachment
  attachments[1].format = depth_format; //depthFormat;
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

  VK_CHECK_RESULT(vkCreateRenderPass(device->device(), &renderPassInfo, nullptr, &renderPass_));
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

}  // namespace lvk