#include "vulkan_renderpass_base.h"

#include <array>

#include "scene.h"
#include "directional_light.h"

#include "vulkan/vulkan_core.h"
#include "vulkan_context.h"
#include "vulkan_tools.h"


namespace lvk {

#define MIN_ALIGNMENT 64
#define UNIFORM_ALIGNMENT(size) ((size + MIN_ALIGNMENT - 1) & ~(MIN_ALIGNMENT - 1))

void VulkanBasePass::SetupRenderPass() {
  VkFormat color_format = context_->GetColorFormat();
  VkFormat depth_format = context_->GetDepthFormat();

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

  VK_CHECK_RESULT(
      vkCreateRenderPass(context_->GetVkDevice(), &renderPassInfo, nullptr, &renderPassData_.renderPassHandle));
}

void VulkanBasePass::SetupFrameBuffer() {
  VkImageView attachments[2];

  // Depth/Stencil attachment is the same for all frame buffers
  attachments[1] = depthStencil_.view;

  VkFramebufferCreateInfo frameBufferCreateInfo = {};
  frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  frameBufferCreateInfo.pNext = NULL;
  frameBufferCreateInfo.renderPass = renderPassData_.renderPassHandle;
  frameBufferCreateInfo.attachmentCount = 2;
  frameBufferCreateInfo.pAttachments = attachments;
  frameBufferCreateInfo.width = context_->width;
  frameBufferCreateInfo.height = context_->height;
  frameBufferCreateInfo.layers = 1;

  // Create frame buffers for every swap chain image
  frameBuffers_.resize(context_->GetSwapChainImageCount());
  for (uint32_t i = 0; i < frameBuffers_.size(); i++) {
    // TODO(andrewli): bug? attachments[i] ?
    attachments[0] = context_->GetSwapChainImageView(i);
    VK_CHECK_RESULT(vkCreateFramebuffer(context_->GetVkDevice(), &frameBufferCreateInfo, nullptr, &frameBuffers_[i]));
  }
}

// TODO: use global proj & view matrix
void VulkanBasePass::PrepareUniformBuffers() {
#if 1
  const auto& vkNodeList = context_->GetVkNodeList();
  const auto& device = context_->GetVulkanDevice();

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

  UpdateUniformBuffers();
#endif
}

void VulkanBasePass::UpdateUniformBuffers() {
  UpdateVertexUniformBuffers();
  UpdateFragmentUniformBuffers();
  UpdateSharedUniformBuffers();
}

void VulkanBasePass::UpdateVertexUniformBuffers() {
  const auto& camera_matrix = scene_->GetCameraMatrix();
  const auto& vkNodeList = context_->GetVkNodeList();
  // update model matrix
  for (size_t i = 0; i < vkNodeList.size(); i++) {
    const auto& vkNode = vkNodeList[i];
    auto& ubo = uniformBuffers_.model[i]; 
    ubo.model = vkNode.sceneNode->ModelMatrix();
    ubo.projection = camera_matrix.proj;
    ubo.view = camera_matrix.view;
  }

  // update to gpu
  uniformBuffers_.vertex_ub.buffer.Update(reinterpret_cast<const uint8_t*>(uniformBuffers_.model),
                                          uniformBuffers_.vertex_ub.buffer_size);
}

void VulkanBasePass::UpdateFragmentUniformBuffers() {
  const auto& vkNodeList = context_->GetVkNodeList();
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

void VulkanBasePass::UpdateSharedUniformBuffers() {
  uniformBuffers_.shared->camera_position = vec4f(scene_->GetCamera()->GetLocation(), 1.0);

  const auto& light_array = scene_->GetAllLights();
  // uniformBuffers_.shared[0].light_num = light_array.size();

  for (auto i = 0; i < light_array.size(); i++) {
    uniformBuffers_.shared->light_direction = vec4f(light_array[i]->GetForwardVector(), 1.0);
    uniformBuffers_.shared->light_color = vec4f(light_array[i]->color(), 1.0);
  }

  // update to gpu
  uniformBuffers_.shared_ub.buffer.Update(reinterpret_cast<const uint8_t*>(uniformBuffers_.shared),
                                          sizeof(*uniformBuffers_.shared));
}

void VulkanBasePass::Prepare() { SetupRenderPass(); }
}  // namespace lvk
