#pragma once

#include <vector>
#include "vulkan/vulkan_core.h"
#include "vulkan_renderpass.h"

namespace lvk {

class VulkanBasePass : public VulkanRenderPass {
 public:
  virtual void Prepare() override;
  virtual void OnSceneChanged() override;

  virtual void BuildCommandBuffer(int cmdBufferIndex, VkCommandBuffer cmdBuffer, const VkCommandBufferBeginInfo* BeginInfo) override;

  VulkanBasePass(VulkanContext* context, Scene* scene, RenderPassType type): VulkanRenderPass(context, scene, type) {}

 private:
  FrameBufferAttachment depthStencil_;
  std::vector<VkFramebuffer> frameBuffers_;

  void SetupDepthStencil();
  void SetupFrameBuffer();
  void SetupRenderPass();
  void SetupDescriptorSet();
  void BuildPipeline();

};

}  // namespace lvk