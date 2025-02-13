#pragma once

#include <vector>

#include "vulkan/vulkan_core.h"


namespace lvk {
class VulkanContext;
class Scene;

enum class RenderPassType {
  BasePass,
  ShadowPass,
};

class VulkanNode;

struct FrameBufferAttachment {
  VkImage image;
  VkDeviceMemory mem;
  VkImageView view;
};

class VulkanRenderPass {
 public:
  struct RenderPassData {
    std::vector<VkFramebuffer> frameBuffers;
    // vulkan raw renderpass handle
    VkRenderPass renderPassHandle{VK_NULL_HANDLE};
    VkPipeline pipelineHandle{VK_NULL_HANDLE};
  };

  VulkanRenderPass(VulkanContext* context, Scene* scene, RenderPassType type);

  virtual void Prepare() {};
  virtual void OnSceneChanged() {};
  virtual void Update() {};

  virtual void BuildCommandBuffer(int cmdBufferIndex, VkCommandBuffer cmdBuffer, const VkCommandBufferBeginInfo* BeginInfo) {};

  const RenderPassData& GetRenderPassData() { return renderPassData_; }

 protected:
  VulkanContext* context_{nullptr};
  Scene* scene_{nullptr};
  // output pass data
  RenderPassData renderPassData_;
};

class VulkanRenderPassBuilder {
 public:
  VulkanRenderPassBuilder() {};
  VulkanRenderPass* Build(VulkanContext* context, Scene* scene, RenderPassType type);
  std::vector<VulkanRenderPass*> BuildAll(VulkanContext* context, Scene* scene);
};
}  // namespace lvk