#pragma once

#include <vector>
#include "vulkan/vulkan_core.h"
#include "vulkan_renderpass.h"

namespace lvk {

class VulkanBasePass : public VulkanRenderPass {
 public:
  virtual void Prepare() override;

  VulkanBasePass(VulkanContext* context, Scene* scene, RenderPassType type): VulkanRenderPass(context, scene, type) {}

 private:
  struct FrameBufferAttachment {
    VkImage image;
    VkDeviceMemory mem;
    VkImageView view;
  } depthStencil_;

  std::vector<VkFramebuffer> frameBuffers_;

  void SetupDepthStencil();
  void SetupFrameBuffer();
  void SetupRenderPass();
};

}  // namespace lvk