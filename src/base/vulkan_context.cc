#include "vulkan_context.h"

#include <corecrt_malloc.h>
#include <stdint.h>

#include <format>
#include <iostream>
#include <vector>

#include "directional_light.h"
#include "lvk_math.h"
#include "node.h"
#include "primitives.h"
#include "scene.h"
#include "vertex_data.h"
#include "vulkan/vulkan_core.h"
#include "vulkan_debug.h"
#include "vulkan_device.h"
#include "vulkan_initializers.h"
#include "vulkan_pipelinebuilder.h"
#include "vulkan_tools.h"
#include "vulkan_renderpass.h"

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
  VulkanRenderPassBuilder builder;
  allRenderPass_ = builder.BuildAll(this, scene);
  // TODO(andrewli):
  basePass_ = allRenderPass_[0];

  Prepare();

  std::cout << std::format("VulkanScene: Total Node: {}\n", scene->GetNodeCount());

  // calc total mesh sections
  size_t num_sections = 0;
  for (int i = 0; i < scene->GetNodeCount(); i++) {
    num_sections += scene->GetResourceMesh(scene->GetNode(i)->mesh)->sections.size();
  }

  vkNodeList.resize(num_sections);
  vkMeshList.resize(num_sections);

  size_t index = 0;
  for (int i = 0; i < scene->GetNodeCount(); i++) {
    const auto& node = scene->GetNode(i);
    const PrimitiveMesh *mesh = scene->GetResourceMesh(node->mesh);
    for (auto j = 0; j < mesh->sections.size(); j++) {
      const MeshSection* section = &mesh->sections[j];

      VulkanNode& vknode = vkNodeList[index];
      PrimitiveMeshVK& vkmesh = vkMeshList[index];
      vknode.vkMesh = &vkMeshList[index];
      vknode.sceneNode = node;

      index++;

      vkmesh.CreateBuffer(section, device);
      vkmesh.indexCount = section->indices.size();

      // std::cout << std::format("VulkanScene: CreateMessBuffer,NodeMesh:{},vkMesh:{}\n", node->mesh, i);
      if (node->materialParamters.textureList.size() > 0) {
        auto texture_handle = node->materialParamters.textureList[0];
        auto texture = new VulkanTexture(device, scene->GetResourceTexture(texture_handle)->path, queue_);
        texture->LoadTexture();
        vkTextureList.push_back(texture);
        vknode.vkTexture = vkTextureList.back();
        vknode.vkTextureHandle = 0;//vkTextureList.size() - 1;
        // std::cout << std::format("VulkanScene: LoadTexture,vkTextureHandle:{}\n", vknode.vkTextureHandle);
      }

      LoadMaterial(&vknode, scene->GetResourceMaterial(node->material), device);
      vknode.pipelineHandle = FindOrCreatePipeline(*node, vknode);
    }
  }

  PrepareUniformBuffers(scene, device);
  SetupDescriptorSetLayout(device);
  BuildPipelines();

  basePass_->OnSceneChanged();

  for (int i = 0; i < vkNodeList.size(); i++) {
    FindOrCreateDescriptorSet(&vkNodeList[i]);
  }
  // SetupDescriptorSet();
}

#define MIN_ALIGNMENT 64
#define UNIFORM_ALIGNMENT(size) ((size + MIN_ALIGNMENT - 1) & ~(MIN_ALIGNMENT - 1))

// TODO: use global proj & view matrix
void VulkanContext::PrepareUniformBuffers(Scene* scene, VulkanDevice* device) {
  size_t bufferSize = vkNodeList.size() * sizeof(_UBOMesh);

  VK_CHECK_RESULT(device->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &uniformBuffers_.vertex_ub.buffer, bufferSize));
  uniformBuffers_.vertex_ub.buffer_size = bufferSize;
  uniformBuffers_.vertex_ub.alignment = sizeof(_UBOMesh);

  size_t fragment_ub_size = vkNodeList.size() * sizeof(_UBOFragment);
  VK_CHECK_RESULT(device->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &uniformBuffers_.fragment_ub.buffer, fragment_ub_size));
  uniformBuffers_.fragment_ub.buffer_size = fragment_ub_size;
  uniformBuffers_.fragment_ub.alignment = sizeof(_UBOFragment);

  // save all model matrix
  // TODO: necessay?
  uniformBuffers_.model = reinterpret_cast<_UBOMesh*>(malloc(bufferSize));
  uniformBuffers_.fragment_data = reinterpret_cast<_UBOFragment*>(malloc(fragment_ub_size));

  // shared
  size_t shared_ub_size = UNIFORM_ALIGNMENT(sizeof(_UBOShared));
  VK_CHECK_RESULT(device->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &uniformBuffers_.shared_ub.buffer, shared_ub_size));
  uniformBuffers_.shared_ub.buffer_size = shared_ub_size;
  uniformBuffers_.shared_ub.alignment = shared_ub_size;
  uniformBuffers_.shared = reinterpret_cast<_UBOShared*>(malloc(shared_ub_size));
  memset(uniformBuffers_.shared, 0, shared_ub_size);

  UpdateUniformBuffers(scene);
}

void VulkanContext::UpdateUniformBuffers(Scene* scene) {
  UpdateVertexUniformBuffers(scene);
  UpdateFragmentUniformBuffers(scene);
  UpdateSharedUniformBuffers(scene);
}

void VulkanContext::UpdateVertexUniformBuffers(Scene* scene) {
  const auto& camera_matrix = scene->GetCameraMatrix();
  // update model matrix
  for (size_t i = 0; i < vkNodeList.size(); i++) {
    const auto& vkNode = vkNodeList[i];
    auto& ubo = uniformBuffers_.model[i];
    ubo.model = vkNode.sceneNode->ModelMatrix();
  }

  // update to gpu
  uniformBuffers_.vertex_ub.buffer.Update(reinterpret_cast<const uint8_t*>(uniformBuffers_.model),
                                          uniformBuffers_.vertex_ub.buffer_size);
}

void VulkanContext::UpdateFragmentUniformBuffers(Scene* scene) {
  // update model matrix
  for (size_t i = 0; i < vkNodeList.size(); i++) {
    const auto& vkNode = vkNodeList[i];
    auto& data = uniformBuffers_.fragment_data[i];
    data.color = vec4f(vkNode.sceneNode->materialParamters.baseColor, 1.0);
    data.metallic = vkNode.sceneNode->materialParamters.metallic;
    data.roughness = vkNode.sceneNode->materialParamters.roughness;
  }

  // update to gpu
  uniformBuffers_.fragment_ub.buffer.Update(reinterpret_cast<const uint8_t*>(uniformBuffers_.fragment_data),
                                            uniformBuffers_.fragment_ub.buffer_size);
}

void VulkanContext::UpdateSharedUniformBuffers(Scene* scene) {
  uniformBuffers_.shared->camera_position = vec4f(scene->GetCamera()->GetLocation(), 1.0);

  const auto& light_array = scene->GetAllLights();
  // uniformBuffers_.shared[0].light_num = light_array.size();

  for (auto i = 0; i < light_array.size(); i++) {
    uniformBuffers_.shared->light_direction = vec4f(light_array[i]->GetForwardVector(), 1.0);
    uniformBuffers_.shared->light_color = vec4f(light_array[i]->color(), 1.0);
  }

  const auto& camera_matrix = scene->GetCameraMatrix();
  uniformBuffers_.shared->projection = camera_matrix.proj;
  uniformBuffers_.shared->view = camera_matrix.view;

  // update to gpu
  uniformBuffers_.shared_ub.buffer.Update(reinterpret_cast<const uint8_t*>(uniformBuffers_.shared),
                                          sizeof(*uniformBuffers_.shared));
}

void VulkanContext::UpdateLightsUniformBuffers(Scene* scene) {}

void VulkanContext::SetupDescriptorSetLayout(VulkanDevice* device) {
  // TODO: pool size 怎么算的?
  // Example uses one ubo and one image sampler
  std::vector<VkDescriptorPoolSize> pool_sizes = {
      initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4),
      initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 16),
      initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2)};

  // TODO: maxSets 怎么算的?
  VkDescriptorPoolCreateInfo descriptor_pool_create_info =
      initializers::DescriptorPoolCreateInfo(static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data(), 16);

  VK_CHECK_RESULT(vkCreateDescriptorPool(device->device(), &descriptor_pool_create_info, nullptr, &descriptorPool_));

  // shared descriptor set layout
  std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
      // global shared uniform buffers
      initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
  };

  VkDescriptorSetLayoutCreateInfo descriptor_layout = initializers::DescriptorSetLayoutCreateInfo(
      set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->device(), &descriptor_layout, nullptr, &descriptorSetLayouts_.shared));

  // object descriptor set layout
  set_layout_bindings = {
      initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT,
                                               0),
      initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
                                               1),
      initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT,
                                               2),
  };

  descriptor_layout = initializers::DescriptorSetLayoutCreateInfo(
      set_layout_bindings.data(), static_cast<uint32_t>(set_layout_bindings.size()));

  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device->device(), &descriptor_layout, nullptr, &descriptorSetLayouts_.object));

  std::array<VkDescriptorSetLayout, 2> layouts = {descriptorSetLayouts_.shared, descriptorSetLayouts_.object};
  VkPipelineLayoutCreateInfo pipeline_layout_create_info =
      initializers::PipelineLayoutCreateInfo(layouts.data(), layouts.size());

  VK_CHECK_RESULT(vkCreatePipelineLayout(device->device(), &pipeline_layout_create_info, nullptr, &pipelineLayout_));

  VkDescriptorSetAllocateInfo allocInfo =
      initializers::DescriptorSetAllocateInfo(descriptorPool_, &descriptorSetLayouts_.shared, 1);
  VK_CHECK_RESULT(vkAllocateDescriptorSets(device_->device(), &allocInfo, &sharedDescriptorSet_));

  auto shared_descriptor = CreateDescriptor(&uniformBuffers_.shared_ub.buffer);
  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      // Binding 0 : shared
      initializers::WriteDescriptorSet(sharedDescriptorSet_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &shared_descriptor),
  };
  vkUpdateDescriptorSets(device_->device(), static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, NULL);
}

#if 0
// TODO: support non-interleaved vertex data
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
#endif

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
  appInfo.apiVersion = VK_API_VERSION_1_2;

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
  if (result == VK_SUCCESS) {
    std::cout << "vkCreateInstance SUCCESS" << std::endl;
  } else {
    std::cout << "vkCreateInstance FAIL" << std::endl;
  }

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

#if 0
void VulkanContext::SetupShadowRenderPass() {
  VkAttachmentDescription attachmentDescription{};
  attachmentDescription.format = shadowPassInfo_.depthFormat;
  attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
  attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
  attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
  attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end

  VkAttachmentReference depthReference = {};
  depthReference.attachment = 0;
  depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;			// Attachment will be used as depth/stencil during render pass

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 0;                   // No color attachments
  subpass.pDepthStencilAttachment = &depthReference;  // Reference to our depth attachment

  // Use subpass dependencies for layout transitions
  std::array<VkSubpassDependency, 2> dependencies;

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo renderPassCreateInfo = initializers::RenderPassCreateInfo();
  renderPassCreateInfo.attachmentCount = 1;
  renderPassCreateInfo.pAttachments = &attachmentDescription;
  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.pSubpasses = &subpass;
  renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassCreateInfo.pDependencies = dependencies.data();

  VK_CHECK_RESULT(vkCreateRenderPass(device_->device(), &renderPassCreateInfo, nullptr, &shadowPassInfo_.renderPass));
}
#endif

void VulkanContext::CreatePipelineCache() {
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  VK_CHECK_RESULT(vkCreatePipelineCache(device_->device(), &pipelineCacheCreateInfo, nullptr, &pipelineCache_));
}

void VulkanContext::BuildPipelines() {
  // todo
  CreatePipelineCache();
  // todo
  // pipelineList.resize(static_cast<std::size_t>(NodeType::MAX));

#if 0
  // for shadowmap
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
      .build(device_->device(), pipelineCache_, PipelineLayout(), renderPass_,
             &pipelineList[static_cast<std::size_t>(NodeType::Object)], "shadowmap");
#endif

#if 0
  std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
  colorBlendAttachmentStates.push_back(initializers::PipelineColorBlendAttachmentState(0xf, VK_FALSE));

  // for general object
  VulkanPipelineBuilder()
      .dynamicStates({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
      .primitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
      .polygonMode(VK_POLYGON_MODE_FILL)
      .vertexInputState(VertexLayout::GetPiplineVertexInputState())
      .cullMode(VK_CULL_MODE_NONE)
      .frontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
      .colorBlendAttachmentStates(colorBlendAttachmentStates)
      .depthWriteEnable(true)
      .depthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
      .rasterizationSamples(VK_SAMPLE_COUNT_1_BIT)
      .shaderStages(GetVkNode(0)->shaderStages)
      .build(device_->device(), pipelineCache_, PipelineLayout(), basePass_->GetRenderPassData().renderPassHandle,
             &pipelineList[static_cast<std::size_t>(NodeType::Object)], "05-GraphicPipline");
#endif
#if 0
  // for skybox
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
      .build(device_->device(), pipelineCache_, PipelineLayout(), renderPass_,
             &pipelineList[static_cast<std::size_t>(NodeType::Skybox)], "05-GraphicPipline");
#endif
}

VkDescriptorSet VulkanContext::AllocDescriptorSet(VulkanNode* vkNode) {
  // todo:
  // descriptorSetList.resize(1);
  std::cout << std::format("AllocDescriptorSet for vkNode: {:010x}\n", (uintptr_t)vkNode);

  VkDescriptorSetAllocateInfo allocInfo =
      initializers::DescriptorSetAllocateInfo(descriptorPool_, &descriptorSetLayouts_.object, 1);

  VkDescriptorSet descriptor_set{VK_NULL_HANDLE};
  VK_CHECK_RESULT(vkAllocateDescriptorSets(device_->device(), &allocInfo, &descriptor_set));

  // Setup a descriptor image info for the current texture to be used as a
  // combined image sampler
  VkDescriptorImageInfo textureDescriptor =
      vkNode->vkTexture->GetDescriptorImageInfo();  // texture_->GetDescriptorImageInfo();

  auto shared_descriptor = CreateDescriptor(&uniformBuffers_.shared_ub.buffer);
  auto dynamic_descriptor = CreateDescriptor(&uniformBuffers_.vertex_ub.buffer, uniformBuffers_.vertex_ub.alignment);
  auto fragment_descriptor =
      CreateDescriptor(&uniformBuffers_.fragment_ub.buffer, uniformBuffers_.fragment_ub.alignment);

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      // Binding 1 : Vertex shader uniform buffer
      initializers::WriteDescriptorSet(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0,
                                       &dynamic_descriptor),
      // Binding 2 : Fragment shader texture sampler
      initializers::WriteDescriptorSet(descriptor_set,
                                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // The descriptor set will
                                                                                   // use a combined image
                                                                                   // sampler (sampler and
                                                                                   // image could be split)
                                       1,                                          // Shader binding point 1
                                       &textureDescriptor),  // Pointer to the descriptor image for our
                                                             // texture
      // Binding3 : Fragment shader uniform buffer
      initializers::WriteDescriptorSet(descriptor_set, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2,
                                       &fragment_descriptor),
  };

  vkUpdateDescriptorSets(device_->device(), static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, NULL);

  // save to cache
  DescriptorSetKey key((void*)&uniformBuffers_, vkNode->vkTextureHandle);
  descriptorSetCache_[key] = descriptor_set;
  return descriptor_set;
}  // namespace lvk

void VulkanContext::FindOrCreateDescriptorSet(VulkanNode* vkNode) {
  vkNode->descriptorSet = AllocDescriptorSet(vkNode);
  #if 0
  DescriptorSetKey key((void*)&uniformBuffers_, vkNode->vkTextureHandle);
  auto iter = descriptorSetCache_.find(key);
  if (iter == descriptorSetCache_.end()) {
    vkNode->descriptorSet = AllocDescriptorSet(vkNode);
  } else {
    vkNode->descriptorSet = iter->second;
  }
  #endif
}

int VulkanContext::FindOrCreatePipeline(const Node& node, const VulkanNode& vkNode) {
  // TODO: 目前假定 pipeline 都是预先创建好了的
  return static_cast<std::size_t>(node.node_type);
}

void VulkanContext::InitSwapchain() { swapChain_.InitSurface(NULL, options_.window); }

void VulkanContext::SetupSwapchain() { swapChain_.Create(&width, &height, false, false); }

#if 0
void VulkanContext::SetupShadowFrameBuffer() {
  // For shadow mapping we only need a depth attachment
  VkImageCreateInfo image = initializers::ImageCreateInfo();
  image.imageType = VK_IMAGE_TYPE_2D;
  image.extent.width = shadowPassInfo_.width;
  image.extent.height = shadowPassInfo_.height;
  image.extent.depth = 1;
  image.mipLevels = 1;
  image.arrayLayers = 1;
  image.samples = VK_SAMPLE_COUNT_1_BIT;
  image.tiling = VK_IMAGE_TILING_OPTIMAL;
  image.format = shadowPassInfo_.depthFormat;  // Depth stencil attachment
  image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT;  // We will sample directly from the depth attachment for the shadow mapping
  VK_CHECK_RESULT(vkCreateImage(device_->device(), &image, nullptr, &shadowPassInfo_.depth.image));

  VkMemoryAllocateInfo memAlloc = initializers::MemoryAllocateInfo();
  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(device_->device(), shadowPassInfo_.depth.image, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = device_->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(vkAllocateMemory(device_->device(), &memAlloc, nullptr, &shadowPassInfo_.depth.mem));
  VK_CHECK_RESULT(vkBindImageMemory(device_->device(), shadowPassInfo_.depth.image, shadowPassInfo_.depth.mem, 0));

  VkImageViewCreateInfo depthStencilView = initializers::ImageViewCreateInfo();
  depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
  depthStencilView.format = shadowPassInfo_.depthFormat;
  depthStencilView.subresourceRange = {};
  depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  depthStencilView.subresourceRange.baseMipLevel = 0;
  depthStencilView.subresourceRange.levelCount = 1;
  depthStencilView.subresourceRange.baseArrayLayer = 0;
  depthStencilView.subresourceRange.layerCount = 1;
  depthStencilView.image = shadowPassInfo_.depth.image;
  VK_CHECK_RESULT(vkCreateImageView(device_->device(), &depthStencilView, nullptr, &shadowPassInfo_.depth.view));

  // Create sampler to sample from to depth attachment
  // Used to sample in the fragment shader for shadowed rendering
  VkFilter shadowmap_filter =
      tools::FormatIsFilterable(device_->physicalDevice(), shadowPassInfo_.depthFormat, VK_IMAGE_TILING_OPTIMAL)
          ? VK_FILTER_LINEAR
          : VK_FILTER_NEAREST;
  VkSamplerCreateInfo sampler = initializers::SamplerCreateInfo();
  sampler.magFilter = shadowmap_filter;
  sampler.minFilter = shadowmap_filter;
  sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sampler.addressModeV = sampler.addressModeU;
  sampler.addressModeW = sampler.addressModeU;
  sampler.mipLodBias = 0.0f;
  sampler.maxAnisotropy = 1.0f;
  sampler.minLod = 0.0f;
  sampler.maxLod = 1.0f;
  sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  VK_CHECK_RESULT(vkCreateSampler(device_->device(), &sampler, nullptr, &shadowPassInfo_.depthSampler));

  // Create frame buffer
  VkFramebufferCreateInfo fbufCreateInfo = initializers::FramebufferCreateInfo();
  fbufCreateInfo.renderPass = shadowPassInfo_.renderPass;
  fbufCreateInfo.attachmentCount = 1;
  fbufCreateInfo.pAttachments = &shadowPassInfo_.depth.view;
  fbufCreateInfo.width = shadowPassInfo_.width;
  fbufCreateInfo.height = shadowPassInfo_.height;
  fbufCreateInfo.layers = 1;
  VK_CHECK_RESULT(vkCreateFramebuffer(device_->device(), &fbufCreateInfo, nullptr, &shadowPassInfo_.frameBuffer));
}
#endif

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

  for (int32_t i = 0; i < drawCmdBuffers_.size(); ++i) {
    VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers_[i], &cmdBufInfo));

    // shadow pass
    {
      VkRenderPassBeginInfo renderPassBeginInfo = initializers::RenderPassBeginInfo();
      renderPassBeginInfo.renderPass = GetBasePassVkHandle();
      renderPassBeginInfo.renderArea.offset.x = 0;
      renderPassBeginInfo.renderArea.offset.y = 0;
      renderPassBeginInfo.renderArea.extent.width = width;
      renderPassBeginInfo.renderArea.extent.height = height;
      renderPassBeginInfo.clearValueCount = 2;
      renderPassBeginInfo.pClearValues = clearValues;
      renderPassBeginInfo.framebuffer = basePass_->GetRenderPassData().frameBuffers[i];
    }

    // base pass starts here
    {
      VkRenderPassBeginInfo renderPassBeginInfo = initializers::RenderPassBeginInfo();
      renderPassBeginInfo.renderPass = GetBasePassVkHandle();
      renderPassBeginInfo.renderArea.offset.x = 0;
      renderPassBeginInfo.renderArea.offset.y = 0;
      renderPassBeginInfo.renderArea.extent.width = width;
      renderPassBeginInfo.renderArea.extent.height = height;
      renderPassBeginInfo.clearValueCount = 2;
      renderPassBeginInfo.pClearValues = clearValues;
      renderPassBeginInfo.framebuffer = basePass_->GetRenderPassData().frameBuffers[i];

      // begin base pass
      vkCmdBeginRenderPass(drawCmdBuffers_[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

      VkViewport viewport = initializers::Viewport((float)width, (float)height, 0.0f, 1.0f);
      vkCmdSetViewport(drawCmdBuffers_[i], 0, 1, &viewport);

      VkRect2D scissor = initializers::Rect2D(width, height, 0, 0);
      vkCmdSetScissor(drawCmdBuffers_[i], 0, 1, &scissor);

      // uint32_t dynamic_offset = 0;
      // vkCmdBindDescriptorSets(drawCmdBuffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout(), 0, 1,
      //                        GetDescriptorSetP(vkNode->descriptorSetHandle), 1, &dynamic_offset);
      vkCmdBindPipeline(drawCmdBuffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                        basePass_->GetRenderPassData().pipelineHandle);

      vkCmdBindDescriptorSets(drawCmdBuffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout(), 0, 1,
                              &sharedDescriptorSet_, 0, nullptr);

      VkDeviceSize offsets[1] = {0};
      for (auto node_index = 0; node_index < vkNodeList.size(); node_index++) {
        const auto& vkn = &vkNodeList[node_index];
        auto vkvb = vkn->vkMesh->vertexBuffer->buffer();
        auto vkib = vkn->vkMesh->indexBuffer->buffer();
        vkCmdBindVertexBuffers(drawCmdBuffers_[i], VERTEX_BUFFER_BIND_ID, 1, &vkvb, offsets);
        vkCmdBindIndexBuffer(drawCmdBuffers_[i], vkib, 0, VK_INDEX_TYPE_UINT32);

        std::array<uint32_t, 2> offset_array;
        offset_array[0] = node_index * uniformBuffers_.vertex_ub.alignment;
        offset_array[1] = node_index * uniformBuffers_.fragment_ub.alignment;
        vkCmdBindDescriptorSets(drawCmdBuffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, PipelineLayout(), 1, 1,
                                &vkn->descriptorSet, 2, &offset_array[0]);
        vkCmdDrawIndexed(drawCmdBuffers_[i], vkn->vkMesh->indexCount, 1, 0, 0, 0);
      }

      RenderComponentBuildCommandBuffers(scene, drawCmdBuffers_[i]);

      // end base pass
      vkCmdEndRenderPass(drawCmdBuffers_[i]);
    }

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

void VulkanContext::Draw(Scene* scene) {
  PrepareFrame();

  UpdateUniformBuffers(scene);

  // Command buffer to be submitted to the queue
  submitInfo_.commandBufferCount = 1;
  submitInfo_.pCommandBuffers = &drawCmdBuffers_[currentBuffer_];

  // Submit to queue
  VK_CHECK_RESULT(vkQueueSubmit(queue_, 1, &submitInfo_, VK_NULL_HANDLE));

  SubmitFrame();

  // UpdateOverlay(scene);
}

void VulkanContext::Prepare() {
  InitSwapchain();
  CreateCommandPool();
  SetupSwapchain();
  CreateCommandBuffers();
  CreateSynchronizationPrimitives();
  #if 0
  SetupDepthStencil();
  SetupRenderPass();
  SetupFrameBuffer();
  #endif
  basePass_->Prepare();

  RenderComponentPrepare();
}

VkPipelineShaderStageCreateInfo VulkanContext::LoadVertexShader(const std::string& path) {
  return LoadShader(path, VK_SHADER_STAGE_VERTEX_BIT, device_);
}

VkPipelineShaderStageCreateInfo VulkanContext::LoadFragmentShader(const std::string& path) {
  return LoadShader(path, VK_SHADER_STAGE_FRAGMENT_BIT, device_);
}

void VulkanContext::RenderComponentPrepare() {
  for (const auto& rc : rc_array_) {
    rc->Prepare(device_, this);
  }
}

void VulkanContext::RenderComponentBuildCommandBuffers(Scene* scene, VkCommandBuffer command_buffer) {
  for (const auto& rc : rc_array_) {
    rc->BuildCommandBuffers(scene, command_buffer);
  }
}

VkDevice VulkanContext::GetVkDevice() {
  return device_->device();
}

VkRenderPass VulkanContext::GetBasePassVkHandle() {
  return basePass_->GetRenderPassData().renderPassHandle;
}
}  // namespace lvk