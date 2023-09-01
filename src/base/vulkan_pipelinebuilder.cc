#include "vulkan_pipelinebuilder.h"

#include "vulkan_debug.h"
#include "vulkan_initializers.h"
#include "vulkan_tools.h"

namespace lvk {

uint32_t VulkanPipelineBuilder::numPipelinesCreated_ = 0;
uint32_t VulkanComputePipelineBuilder::numPipelinesCreated_ = 0;

VulkanPipelineBuilder::VulkanPipelineBuilder()
    : vertexInputState_(initializers::PipelineVertexInputStateCreateInfo()),
      inputAssembly_(
          initializers::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE)),
      rasterizationState_(initializers::PipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE,
                                                                             VK_FRONT_FACE_COUNTER_CLOCKWISE)),
      multisampleState_(initializers::PipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0)),
      depthStencilState_(initializers::PipelineDepthStencilStateCreateInfo()) {}

VulkanPipelineBuilder& VulkanPipelineBuilder::depthBiasEnable(bool enable) {
  rasterizationState_.depthBiasEnable = enable ? VK_TRUE : VK_FALSE;
  return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::depthWriteEnable(bool enable) {
  depthStencilState_.depthWriteEnable = enable ? VK_TRUE : VK_FALSE;
  return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::depthCompareOp(VkCompareOp compareOp) {
  depthStencilState_.depthTestEnable = compareOp != VK_COMPARE_OP_ALWAYS;
  depthStencilState_.depthCompareOp = compareOp;
  return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::dynamicState(VkDynamicState state) {
  dynamicStates_.push_back(state);
  return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::dynamicStates(const std::vector<VkDynamicState>& states) {
  dynamicStates_.insert(std::end(dynamicStates_), std::begin(states), std::end(states));
  return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::primitiveTopology(VkPrimitiveTopology topology) {
  inputAssembly_.topology = topology;
  return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::rasterizationSamples(VkSampleCountFlagBits samples) {
  multisampleState_.rasterizationSamples = samples;
  return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::cullMode(VkCullModeFlags mode) {
  rasterizationState_.cullMode = mode;
  return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::frontFace(VkFrontFace mode) {
  rasterizationState_.frontFace = mode;
  return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::polygonMode(VkPolygonMode mode) {
  rasterizationState_.polygonMode = mode;
  return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::vertexInputState(const VkPipelineVertexInputStateCreateInfo& state) {
  vertexInputState_ = state;
  return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::colorBlendAttachmentStates(
    std::vector<VkPipelineColorBlendAttachmentState>& states) {
  colorBlendAttachmentStates_ = std::move(states);
  return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::shaderStage(VkPipelineShaderStageCreateInfo stage) {
  shaderStages_.push_back(stage);
  return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::shaderStages(const std::vector<VkPipelineShaderStageCreateInfo>& stages) {
  shaderStages_.insert(std::end(shaderStages_), std::begin(stages), std::end(stages));
  return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::stencilStateOps(VkStencilFaceFlags faceMask, VkStencilOp failOp,
                                                              VkStencilOp passOp, VkStencilOp depthFailOp,
                                                              VkCompareOp compareOp) {
  if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
    VkStencilOpState& front = depthStencilState_.front;
    front.failOp = failOp;
    front.passOp = passOp;
    front.depthFailOp = depthFailOp;
    front.compareOp = compareOp;
  }
  if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
    VkStencilOpState& back = depthStencilState_.back;
    back.failOp = failOp;
    back.passOp = passOp;
    back.depthFailOp = depthFailOp;
    back.compareOp = compareOp;
  }
  return *this;
}

VkResult VulkanPipelineBuilder::build(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout pipelineLayout,
                                      VkRenderPass renderPass, VkPipeline* outPipeline,
                                      const char* debugName) noexcept {
  const VkPipelineDynamicStateCreateInfo dynamicState =
      initializers::PipelineDynamicStateCreateInfo(dynamicStates_.data(), (uint32_t)dynamicStates_.size());

  // viewport and scissor are always dynamic
  const VkPipelineViewportStateCreateInfo viewportState = initializers::PipelineViewportStateCreateInfo(1, 1);

  const VkPipelineColorBlendStateCreateInfo colorBlendState = initializers::PipelineColorBlendStateCreateInfo(
      uint32_t(colorBlendAttachmentStates_.size()), colorBlendAttachmentStates_.data());

  const auto result = initializers::CreateGraphicsPipeline(
      device, pipelineCache, (uint32_t)shaderStages_.size(), shaderStages_.data(), &vertexInputState_, &inputAssembly_,
      nullptr, &viewportState, &rasterizationState_, &multisampleState_, &depthStencilState_, &colorBlendState,
      &dynamicState, pipelineLayout, renderPass, outPipeline);

  VK_CHECK_RESULT(result);

  numPipelinesCreated_++;

  // set debug name
  return debugutils::SetDebugObjectName(device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)*outPipeline, debugName);
}

VulkanComputePipelineBuilder& VulkanComputePipelineBuilder::shaderStage(VkPipelineShaderStageCreateInfo stage) {
  shaderStage_ = stage;
  return *this;
}

VkResult VulkanComputePipelineBuilder::build(VkDevice device, VkPipelineCache pipelineCache,
                                             VkPipelineLayout pipelineLayout, VkPipeline* outPipeline,
                                             const char* debugName) noexcept {
  const VkResult result =
      initializers::CreateComputePipeline(device, pipelineCache, &shaderStage_, pipelineLayout, outPipeline);
  VK_CHECK_RESULT(result);

  numPipelinesCreated_++;

  // set debug name
  return debugutils::SetDebugObjectName(device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)*outPipeline, debugName);
}

}  // namespace lvk
