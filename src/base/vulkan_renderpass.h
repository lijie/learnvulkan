#include "vulkan/vulkan_core.h"

namespace lvk {
class VulkanContext;
class Scene;

enum class RenderPassType {
  BasePass,
  ShadowPass,
};

class VulkanRenderPass {
 public:
  struct RenderPassData {
    // vulkan raw renderpass handle
    VkRenderPass renderPassHandle{VK_NULL_HANDLE};
  };

  virtual void Prepare();
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
  VulkanRenderPass* Build(VulkanContext* context, Scene *scene);
};
}  // namespace lvk