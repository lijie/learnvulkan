#pragma once

#include <array>
#include <string>
#include <vector>

#include "primitives.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include "vulkan_buffer.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_texture.h"

namespace lvk {
class VulkanDevice;
class Scene;
class Material;
class Window;

struct VulkanContextOptions {
  Window *window{nullptr};
  VkFormat depthFormat{VK_FORMAT_UNDEFINED};
};

struct VulkanNode {
  PrimitiveMeshVK *vkMesh{nullptr};
  VulkanTexture *vkTexture{nullptr};
  // alias shaders
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  int pipelineHandle{0};
  int descriptorSetHandle{0};
};

class VulkanContext {
 public:
  // todo:
  VulkanContext();
  ~VulkanContext();

  // todo:
  void InitWithOptions(const VulkanContextOptions &options, VkPhysicalDevice phy_device);

  void CreateVulkanScene(Scene *scene, VulkanDevice *device);

  void LoadTextures(Scene *scene, VulkanDevice *device);
  void PrepareUniformBuffers(Scene *scene, VulkanDevice *device);
  void UpdateUniformBuffers(Scene *scene);
  void SetupDescriptorPool(VulkanDevice *device);
  void SetupDescriptorSetLayout(VulkanDevice *device);
  void SetupDescriptorSet();
  void SetupRenderPass();
  void CreatePipelineCache();
  void BuildPipelines();

  void CreateCommandPool();
  void CreateCommandBuffers();
  void BuildCommandBuffers(Scene *scene);
  void CreateSynchronizationPrimitives();

  void SetupFrameBuffer();
  void SetupDepthStencil();

  void InitSwapchain();
  void SetupSwapchain();

  void PrepareFrame();
  void SubmitFrame();
  void Draw();

  void Prepare();

  const VkPipelineVertexInputStateCreateInfo &BuildVertexInputState();

  VkInstance instance() const { return instance_; }
  VkRenderPass renderPass() const { return renderPass_; }

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

  const VulkanNode *GetVkNode(int handle) { return &vkNodeList[handle]; }
  VkPipeline GetPipeline(int handle) { return pipelineList[handle]; }
  VkDescriptorSet *GetDescriptorSetP(int handle) { return &descriptorSetList[handle]; }

  void set_vulkan_device(VulkanDevice *device) { device_ = device; };

  uint32_t width{1280};
  uint32_t height{720};

 private:
  VulkanContextOptions options_;
  VkInstance instance_{VK_NULL_HANDLE};
  VulkanSwapchain swapChain_;
  VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};
  // VkDescriptorSet descriptorSet_{VK_NULL_HANDLE};
  VkDescriptorSetLayout descriptorSetLayout_{VK_NULL_HANDLE};
  VkPipelineLayout pipelineLayout_{VK_NULL_HANDLE};
  VkRenderPass renderPass_ = VK_NULL_HANDLE;
  VulkanDevice *device_{nullptr};
  VkPipelineCache pipelineCache_{VK_NULL_HANDLE};

  std::vector<VkFramebuffer> frameBuffers_;
  uint32_t currentBuffer_ = 0;
  VkQueue queue_;

  VkCommandPool cmdPool_;
  std::vector<VkCommandBuffer> drawCmdBuffers_;
  std::vector<VkFence> waitFences_;

  std::vector<std::string> supportedInstanceExtensions;
  /** @brief Set of device extensions to be enabled for this example (must be
   * set in the derived constructor) */
  std::vector<const char *> enabledDeviceExtensions;
  std::vector<const char *> enabledInstanceExtensions;

  struct {
    VkImage image;
    VkDeviceMemory mem;
    VkImageView view;
  } depthStencil_;

  struct {
    // Swap chain image presentation
    VkSemaphore presentComplete;
    // Command buffer submission and execution
    VkSemaphore renderComplete;
  } semaphores_;
  VkSubmitInfo submitInfo_;
  VkPipelineStageFlags submitPipelineStages_ = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

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

  std::vector<VkPipeline> pipelineList;
  std::vector<VkDescriptorSet> descriptorSetList;

  std::vector<VulkanTexture *> vkTextureList;
  std::vector<PrimitiveMeshVK> vkMeshList;
  std::vector<VulkanNode> vkNodeList;

  VkResult CreateInstance(bool enableValidation);
  VkPipelineShaderStageCreateInfo LoadShader(std::string fileName, VkShaderStageFlagBits stage, VulkanDevice *device);
  bool LoadMaterial(VulkanNode *vkNode, const Material *mat, VulkanDevice *device);
  int FindOrCreatePipeline() { return 0; }
  // void BuildPipelines();
};
}  // namespace lvk
