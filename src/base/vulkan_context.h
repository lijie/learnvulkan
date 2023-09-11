#pragma once

#include <array>
#include <string>
#include <vector>

#include "primitives.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include "vulkan_buffer.h"
#include "vulkan_device.h"

namespace lvk {
class VulkanDevice;
class Scene;
class VulkanContext {
 public:
  VulkanContext();
  ~VulkanContext();

  void PrepareUniformBuffers(Scene *scene, VulkanDevice *device);
  void UpdateUniformBuffers(Scene *scene);
  void SetupDescriptorPool(VulkanDevice *device);
  void SetupDescriptorSetLayout(VulkanDevice *device);
  void SetupDescriptorSet(VulkanDevice *device);

  const VkPipelineVertexInputStateCreateInfo &BuildVertexInputState();

  VkInstance instance() const { return instance_; }

  // helpers
  VkDescriptorBufferInfo CreateDescriptor(VulkanBuffer *buffer, VkDeviceSize size = VK_WHOLE_SIZE,
                                          VkDeviceSize offset = 0);

  // temp test
  VkDescriptorBufferInfo CreateDescriptor2() {
    return CreateDescriptor(&uniformBuffers_.dynamic, uniformBuffers_.dynamicAlignment);
  }

  VkDescriptorPool DescriptorPool() { return descriptorPool_; }
  const VkDescriptorSetLayout *DescriptorSetLayout() { return &descriptorSetLayout_; }
  VkPipelineLayout PipelineLayout() { return pipelineLayout_; }

 private:
  VkInstance instance_{VK_NULL_HANDLE};
  VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};
  VkDescriptorSet descriptorSet_{VK_NULL_HANDLE};
  VkDescriptorSetLayout descriptorSetLayout_{VK_NULL_HANDLE};
  VkPipelineLayout pipelineLayout_{VK_NULL_HANDLE};
  std::vector<std::string> supportedInstanceExtensions;
  /** @brief Set of device extensions to be enabled for this example (must be
   * set in the derived constructor) */
  std::vector<const char *> enabledDeviceExtensions;
  std::vector<const char *> enabledInstanceExtensions;

  struct _UBOMesh {
    glm::mat4 projection;
    glm::mat4 modelView;
  };

  struct _UniformBuffers {
    _UBOMesh *model{nullptr};
    VulkanBuffer view;
    VulkanBuffer dynamic;
    size_t dynamicBufferSize{0};
    size_t dynamicAlignment{0};
  } uniformBuffers_;

  struct _VertexInputState {
    std::array<VkVertexInputBindingDescription, 1> bindingDescriptions;
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;
    VkPipelineVertexInputStateCreateInfo inputState;
  } vertexInputState_;

  VkResult CreateInstance(bool enableValidation);
};
}  // namespace lvk
