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

void VulkanBasePass::SetupDepthStencil() {
  VkImageCreateInfo imageCI{};
  imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCI.imageType = VK_IMAGE_TYPE_2D;
  imageCI.format = context_->GetDepthFormat();
  imageCI.extent = {context_->width, context_->height, 1};
  imageCI.mipLevels = 1;
  imageCI.arrayLayers = 1;
  imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VK_CHECK_RESULT(vkCreateImage(context_->GetVkDevice(), &imageCI, nullptr, &depthStencil_.image));
  VkMemoryRequirements memReqs{};
  vkGetImageMemoryRequirements(context_->GetVkDevice(), depthStencil_.image, &memReqs);

  VkMemoryAllocateInfo memAllloc{};
  memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memAllloc.allocationSize = memReqs.size;
  memAllloc.memoryTypeIndex = context_->GetVulkanDevice()->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(vkAllocateMemory(context_->GetVkDevice(), &memAllloc, nullptr, &depthStencil_.mem));
  VK_CHECK_RESULT(vkBindImageMemory(context_->GetVkDevice(), depthStencil_.image, depthStencil_.mem, 0));

  VkImageViewCreateInfo imageViewCI{};
  imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewCI.image = depthStencil_.image;
  imageViewCI.format = context_->GetDepthFormat();
  imageViewCI.subresourceRange.baseMipLevel = 0;
  imageViewCI.subresourceRange.levelCount = 1;
  imageViewCI.subresourceRange.baseArrayLayer = 0;
  imageViewCI.subresourceRange.layerCount = 1;
  imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  // Stencil aspect should only be set on depth + stencil formats
  // (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
  if (context_->GetDepthFormat() >= VK_FORMAT_D16_UNORM_S8_UINT) {
    imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }
  VK_CHECK_RESULT(vkCreateImageView(context_->GetVkDevice(), &imageViewCI, nullptr, &depthStencil_.view));
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

  renderPassData_.frameBuffers = frameBuffers_;
}


void VulkanBasePass::Prepare() {
  SetupDepthStencil();
  SetupRenderPass();
  SetupFrameBuffer();
}

}  // namespace lvk
