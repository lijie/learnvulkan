#include "vulkan_renderpass.h"

#include "vulkan_renderpass_base.h"
#include "vulkan_renderpass_shadow.h"


namespace lvk {

VulkanRenderPass::VulkanRenderPass(VulkanContext* context, Scene* scene, RenderPassType type) {
  context_ = context;
  scene_ = scene;
}

VulkanRenderPass* VulkanRenderPassBuilder::Build(VulkanContext* context, Scene* scene, RenderPassType type) {
  switch (type) {
    case RenderPassType::BasePass:
      return new VulkanBasePass(context, scene, type);
      break;
    case lvk::RenderPassType::ShadowPass:
      return new VulkanShadowPass(context, scene, type);
      break;
    default:
      break;
  }

  return nullptr;
}

std::vector<VulkanRenderPass*> VulkanRenderPassBuilder::BuildAll(VulkanContext* context, Scene* scene) {
  RenderPassType type_array[] = {
      RenderPassType::BasePass,
  };

  std::vector<VulkanRenderPass*> pass_vec;
  for (auto type : type_array) {
    pass_vec.push_back(Build(context, scene, type));
  }

  return pass_vec;
}

}  // namespace lvk
