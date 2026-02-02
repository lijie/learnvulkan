#pragma once

#include <vector>
#include "vulkan/vulkan_core.h"
#include "vulkan_renderpass.h"

namespace lvk {

class VulkanShadowPass : public VulkanRenderPass {
 public:
  virtual void Prepare() override;
  virtual void OnSceneChanged() override;

  virtual void BuildCommandBuffer(int cmdBufferIndex, VkCommandBuffer cmdBuffer, const VkCommandBufferBeginInfo* BeginInfo) override;

  VulkanShadowPass(VulkanContext* context, Scene* scene, RenderPassType type): VulkanRenderPass(context, scene, type) {}

 private:
  FrameBufferAttachment depthStencil_;
  std::vector<VkFramebuffer> frameBuffers_;
  VkFormat depthFormat_{VK_FORMAT_D16_UNORM};

  void SetupDepthStencil();
  void SetupFrameBuffer();
  void SetupRenderPass();
  void SetupDescriptorSet();
  void BuildPipeline();
};

}  // namespace lvk