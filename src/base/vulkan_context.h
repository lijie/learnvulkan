#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

#include "lvk_math.h"
#include "primitives.h"
#include "node.h"
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
class VulkanContext;

enum class PipelineType {
  Shadow,
  General,
};

enum class SharedDescriptorSetLayoutBinding { 
  UBOShared = 0,
  UBOVP = 1,
};

enum class PerObjectDescriptorSetLayoutBinding { 
  UBOModel = 0,
  TextureSampler = 1,
};

// 用于在渲染流程中插入自定义渲染逻辑
class RenderComponent {
  public:
    virtual void Prepare(VulkanDevice *device, VulkanContext *context) {};
    virtual void BuildCommandBuffers(Scene *scene, VkCommandBuffer command_buffer) {};
};

struct VulkanContextOptions {
  Window *window{nullptr};
  VkFormat depthFormat{VK_FORMAT_UNDEFINED};
};

struct VulkanNode {
  PrimitiveMeshVK *vkMesh{nullptr};
  VulkanTexture *vkTexture{nullptr};
  int vkTextureHandle{0};
  // alias shaders
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  int pipelineHandle{0};
  // int descriptorSetHandle{0};
  VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
  // ref to logic Node in scene
  Node *sceneNode{nullptr};
};

// 如果使用不同的 unitofrm 或者不同的 texture,
// 则需要创建新的 DescriptorSet
struct DescriptorSetKey {
  DescriptorSetKey() {}
  DescriptorSetKey(void *ptr, int handle) : uniformBufferPtr(ptr), textureHandle(handle) {}
  void *uniformBufferPtr;
  int textureHandle;

  bool operator==(const DescriptorSetKey &other) const {
    if (this->uniformBufferPtr == other.uniformBufferPtr && this->textureHandle == other.textureHandle)
      return true;
    else
      return false;
  }

  struct HashFunction {
    size_t operator()(const DescriptorSetKey &key) const {
      size_t rowHash = std::hash<void *>()(key.uniformBufferPtr);
      size_t colHash = std::hash<int>()(key.textureHandle) << 1;
      return rowHash ^ colHash;
    }
  };
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
  void UpdateVertexUniformBuffers(Scene *scene);
  void UpdateFragmentUniformBuffers(Scene *scene);
  void UpdateSharedUniformBuffers(Scene *scene);
  void UpdateLightsUniformBuffers(Scene *scene);

  void SetupDescriptorSetLayout(VulkanDevice *device);
  VkDescriptorSet AllocDescriptorSet(VulkanNode *vkNode);
  void CreatePipelineCache();
  void BuildPipelines();

  void CreateCommandPool();
  void CreateCommandBuffers();
  void BuildCommandBuffers(Scene *scene);
  void CreateSynchronizationPrimitives();

  void InitSwapchain();
  void SetupSwapchain();

  void PrepareFrame();
  void SubmitFrame();
  void Draw(Scene *scene);

  void Prepare();

  // utils
  VkPipelineShaderStageCreateInfo LoadVertexShader(const std::string& path);
  VkPipelineShaderStageCreateInfo LoadFragmentShader(const std::string& path);

  VkInstance instance() const { return instance_; }
  // VkRenderPass renderPass() const { return renderPass_; }

  // helpers
  VkDescriptorBufferInfo CreateDescriptor(VulkanBuffer *buffer, VkDeviceSize size = VK_WHOLE_SIZE,
                                          VkDeviceSize offset = 0);

  // temp test
  // VkDescriptorBufferInfo CreateDescriptor2() {
  //   return CreateDescriptor(&uniformBuffers_.dynamic, uniformBuffers_.dynamicAlignment);
  // }

  VkDescriptorPool DescriptorPool() { return descriptorPool_; }
  // const VkDescriptorSetLayout *DescriptorSetLayout() { return &descriptorSetLayout_; }
  VkPipelineLayout PipelineLayout() { return pipelineLayout_; }

  const VulkanNode *GetVkNode(int handle) { return &vkNodeList[handle]; }
  // VkPipeline GetPipeline(int handle) { return pipelineList[handle]; }
  VkQueue GetQueue() { return queue_; }
  VkPipelineCache GetPipelineCache() { return pipelineCache_; }
  // VkRenderPass GetRenderPass() { return renderPass_; }
  VkRenderPass GetBasePassVkHandle();
  VkFormat GetColorFormat() { return swapChain_.colorFormat(); }
  VkFormat GetDepthFormat() { return options_.depthFormat; }
  size_t GetSwapChainImageCount() { return swapChain_.imageCount_; }
  VkImageView GetSwapChainImageView(size_t i) { return swapChain_.buffers_[i].view; }

  // return vulkan device wrapper
  VulkanDevice* GetVulkanDevice() { return device_; };
  // return raw vulkan device
  VkDevice GetVkDevice();

  const std::vector<VulkanNode>& GetVkNodeList() { return vkNodeList; };

  void set_vulkan_device(VulkanDevice *device) { device_ = device; };
  void AddRenderComponent(RenderComponent *rc) { rc_array_.push_back(rc); }

  uint32_t width{1280};
  uint32_t height{720};

 private:
  VulkanContextOptions options_;
  VkInstance instance_{VK_NULL_HANDLE};
  VulkanSwapchain swapChain_;
  VkDescriptorPool descriptorPool_{VK_NULL_HANDLE};
  // VkDescriptorSet descriptorSet_{VK_NULL_HANDLE};
  // VkDescriptorSetLayout descriptorSetLayout_{VK_NULL_HANDLE};
  // VkDescriptorSetLayout sharedDescriptorSetLayout_{VK_NULL_HANDLE};

  // 我们创建2个 DescriptorSet, 一个用于共享数据, 比如VP矩阵, 光照数据, 一个用于对象数据, 比如Model矩阵
  struct DescriptorSetLayouts {
	  VkDescriptorSetLayout shared{ VK_NULL_HANDLE };
	  VkDescriptorSetLayout object{ VK_NULL_HANDLE };
    // extend here?
  } descriptorSetLayouts_;

  VkDescriptorSet sharedDescriptorSet_{VK_NULL_HANDLE};

  VkPipelineLayout pipelineLayout_{VK_NULL_HANDLE};
  // VkRenderPass renderPass_ = VK_NULL_HANDLE;
  VulkanDevice *device_{nullptr};
  VkPipelineCache pipelineCache_{VK_NULL_HANDLE};

  // std::vector<VkFramebuffer> frameBuffers_;
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

  // struct FrameBufferAttachment {
  //   VkImage image;
  //   VkDeviceMemory mem;
  //   VkImageView view;
  // } depthStencil_;

  struct {
    // Swap chain image presentation
    VkSemaphore presentComplete;
    // Command buffer submission and execution
    VkSemaphore renderComplete;
  } semaphores_;
  VkSubmitInfo submitInfo_;
  VkPipelineStageFlags submitPipelineStages_ = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  struct _UBOMesh {
    glm::mat4 model;
  };

  struct _UBOFragment {
    vec4f color;
    float roughness;
    float metallic;
    float padding[2];
    float padding2[8];
  };

  struct _UBOShared {
    vec4f camera_position;
    vec4f light_direction;
    vec4f light_color;
    mat4f light_mvp;
    mat4f projection;
    mat4f view;
  };

  struct UniformBuffer {
    VulkanBuffer buffer;
    size_t buffer_size{0};
    size_t alignment{0};
  };

  struct _UniformBuffers {
    _UBOMesh *model{nullptr};
    _UBOFragment *fragment_data{nullptr};
    _UBOShared *shared;
    // uniform buffer for global proj & view matrix
    // VulkanBuffer view;

    UniformBuffer shared_ub;
    // uniform buffer in vertex shader, per object data, all packed here
    UniformBuffer vertex_ub;
    // uniform buffer in pixel shader, per object data, all packed here
    UniformBuffer fragment_ub;

    // VulkanBuffer dynamic;
    // size_t dynamicBufferSize{0};
    // size_t dynamicAlignment{0};
    // VulkanBuffer fragment;
  } uniformBuffers_;

  struct _VertexInputState {
    std::array<VkVertexInputBindingDescription, 1> bindingDescriptions;
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;
    VkPipelineVertexInputStateCreateInfo inputState;
  } vertexInputState_;

  // std::vector<VkPipeline> pipelineList;
  // std::vector<VkDescriptorSet> descriptorSetList;

  std::vector<VulkanTexture *> vkTextureList;
  std::vector<PrimitiveMeshVK> vkMeshList;
  std::vector<VulkanNode> vkNodeList;

  // caches
  // std::map<DescriptorSetKey, VkDescriptorSet> descriptorSetCache_;
  std::unordered_map<DescriptorSetKey, VkDescriptorSet, DescriptorSetKey::HashFunction> descriptorSetCache_;

  std::vector<RenderComponent*> rc_array_;

#if 0
  struct ShadowPassInfo {
	  int32_t width, height;
	  VkFramebuffer frameBuffer;
	  FrameBufferAttachment depth;
	  VkRenderPass renderPass;
	  VkSampler depthSampler;
	  VkDescriptorImageInfo descriptor;
    VkFormat depthFormat{VK_FORMAT_D16_UNORM};
  } shadowPassInfo_{};
#endif

  std::vector<class VulkanRenderPass*> allRenderPass_;
  class VulkanRenderPass* basePass_{nullptr};

  VkResult CreateInstance(bool enableValidation);
  VkPipelineShaderStageCreateInfo LoadShader(std::string fileName, VkShaderStageFlagBits stage, VulkanDevice *device);
  bool LoadMaterial(VulkanNode *vkNode, const Material *mat, VulkanDevice *device);

  void BuildLinePipeline();
  int FindOrCreatePipeline(const Node& node, const VulkanNode& vkNode);
  void FindOrCreateDescriptorSet(VulkanNode *vkNode);
  // void BuildPipelines();
  void UpdateOverlay(Scene* scene);

  void RenderComponentPrepare();
  void RenderComponentBuildCommandBuffers(Scene* scene, VkCommandBuffer command_buffer);
};
}  // namespace lvk
