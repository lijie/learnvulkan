#pragma once

#include "vulkan/vulkan.h"
#include <vector>
#include "lvk_math.h"
#include "vulkan_context.h"

namespace lvk {

class VulkanDevice;
class VulkanBuffer;

class VulkanUI {
 public:
  VulkanDevice* device{nullptr};
  VkQueue queue{VK_NULL_HANDLE};

  VkSampleCountFlagBits rasterizationSamples{VK_SAMPLE_COUNT_1_BIT};
  uint32_t subpass{0};

  VulkanBuffer* vertexBuffer{nullptr};
  VulkanBuffer* indexBuffer{nullptr};
  int32_t vertexCount{0};
  int32_t indexCount{0};

  std::vector<VkPipelineShaderStageCreateInfo> shaders;

  VkDescriptorPool descriptorPool{VK_NULL_HANDLE};
  VkDescriptorSetLayout descriptorSetLayout{VK_NULL_HANDLE};
  VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
  VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};
  VkPipeline pipeline{VK_NULL_HANDLE};

  VkDeviceMemory fontMemory{VK_NULL_HANDLE};
  VkImage fontImage{VK_NULL_HANDLE};
  VkImageView fontView{VK_NULL_HANDLE};
  VkSampler sampler{VK_NULL_HANDLE};

  struct PushConstBlock {
    lvk::vec2f scale;
    lvk::vec2f translate;
  } pushConstBlock;

  bool visible{true};
  bool updated{false};
  float scale{1.0f};
  float updateTimer{0.0f};

  VulkanUI();
  ~VulkanUI();

  void PreparePipeline(const VkPipelineCache pipelineCache, const VkRenderPass renderPass, const VkFormat colorFormat,
                       const VkFormat depthFormat);
  void PrepareResources();

  bool Update();
  void draw(const VkCommandBuffer commandBuffer);
  void resize(uint32_t width, uint32_t height);

  void freeResources();

  bool header(const char* caption);
  bool checkBox(const char* caption, bool* value);
  bool checkBox(const char* caption, int32_t* value);
  bool radioButton(const char* caption, bool value);
  bool inputFloat(const char* caption, float* value, float step, const char* format);
  bool sliderFloat(const char* caption, float* value, float min, float max);
  bool sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);
  bool comboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items);
  bool button(const char* caption);
  bool colorPicker(const char* caption, float* color);
  void text(const char* formatstr, ...);
};

class VulkanUIRenderWrapper : public RenderComponent {
  public:
    VulkanUIRenderWrapper(VulkanUI *ui, float w, float h): ui_(ui), width_(w), height_(h) {};

    virtual void Prepare(VulkanDevice *device, VulkanContext *context) override;
    virtual void BuildCommandBuffers(Scene *scene, VkCommandBuffer command_buffer) override;

  protected:
    VulkanUI *ui_{nullptr};
    float width_{0.0};
    float height_{0.0};
};

}  // namespace lkv
