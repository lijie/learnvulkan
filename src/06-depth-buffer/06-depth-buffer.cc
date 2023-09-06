#include <array>
#include <iostream>

#include "base/vertex_data.h"
#include "base/vulkan_app.h"
#include "base/vulkan_buffer.h"
#include "base/vulkan_device.h"
#include "base/vulkan_initializers.h"
#include "base/vulkan_pipelinebuilder.h"
#include "base/vulkan_texture.h"
#include "base/vulkan_tools.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#include "glm/glm.hpp"

#define VERTEX_BUFFER_BIND_ID 0

namespace lvk {
using lvk::VulkanApp;

// Vertex layout for this example
#if 0
struct Vertex {
  float pos[3];
  float uv[2];
  float normal[3];
};
#endif

class DepthBufferApp : public VulkanApp {
 private:
  VulkanTexture *texture_{nullptr};

  lvk::VulkanBuffer uniformBufferVS;

  struct {
    glm::mat4 projection;
    glm::mat4 modelView;
    glm::vec4 viewPos;
    float lodBias = 0.0f;
  } uboVS;

  struct {
    VkPipeline solid;
  } pipelines;

  VkPipelineLayout pipelineLayout;
  VkDescriptorSet descriptorSet;
  VkDescriptorSetLayout descriptorSetLayout;

  VkClearColorValue defaultClearColor = {{0.025f, 0.025f, 0.025f, 1.0f}};

  void LoadTexture();
  void GenerateQuad();
  void PrepareUniformBuffers();
  void UpdateUniformBuffers();
  void SetupDescriptorSetLayout();
  void PreparePipelines();
  void SetupDescriptorPool();
  void SetupDescriptorSet();
  void BuildCommandBuffers();
  void Draw();

 public:
  void Render();
  virtual void Prepare() override;
};

void DepthBufferApp::GenerateQuad() {
  QuadMesh::instance()->InitForRendering(vulkanDevice);
}

void DepthBufferApp::UpdateUniformBuffers() {
  uboVS.projection = glm::perspective(glm::radians(45.0f), float(width / height), 0.1f, 10.0f);

  auto view = glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  uboVS.modelView = view * glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  // uboVS.viewPos = camera.viewPos;
  memcpy(uniformBufferVS.mapped(), &uboVS, sizeof(uboVS));
}

// Prepare and initialize uniform buffer containing shader uniforms
void DepthBufferApp::PrepareUniformBuffers() {
  // Vertex shader uniform buffer block
  VK_CHECK_RESULT(vulkanDevice->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                             &uniformBufferVS, sizeof(uboVS), &uboVS));
  VK_CHECK_RESULT(uniformBufferVS.Map());

  UpdateUniformBuffers();
}

void DepthBufferApp::SetupDescriptorSetLayout() {
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      // Binding 0 : Vertex shader uniform buffer
      lvk::initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
      // Binding 1 : Fragment shader image sampler
      lvk::initializers::DescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                    VK_SHADER_STAGE_FRAGMENT_BIT, 1)};

  VkDescriptorSetLayoutCreateInfo descriptorLayout = lvk::initializers::DescriptorSetLayoutCreateInfo(
      setLayoutBindings.data(), static_cast<uint32_t>(setLayoutBindings.size()));

  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

  VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
      lvk::initializers::PipelineLayoutCreateInfo(&descriptorSetLayout, 1);

  VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, nullptr, &pipelineLayout));
}

void DepthBufferApp::PreparePipelines() {
  std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
  colorBlendAttachmentStates.push_back(initializers::PipelineColorBlendAttachmentState(0xf, VK_FALSE));

  // Load shaders
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  shaderStages.push_back(LoadShader(GetShadersPath() + "05-vert.spv", VK_SHADER_STAGE_VERTEX_BIT));
  shaderStages.push_back(LoadShader(GetShadersPath() + "05-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT));

  VulkanPipelineBuilder()
      .dynamicStates({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
      .primitiveTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
      .polygonMode(VK_POLYGON_MODE_FILL)
      .vertexInputState(QuadMesh::instance()->GetVkPipelineVertexInputStateCreateInfo())
      .cullMode(VK_CULL_MODE_NONE)
      .frontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
      .colorBlendAttachmentStates(colorBlendAttachmentStates)
      .depthWriteEnable(true)
      .depthCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL)
      .rasterizationSamples(VK_SAMPLE_COUNT_1_BIT)
      .shaderStages(shaderStages)
      .build(device, pipelineCache, pipelineLayout, renderPass, &pipelines.solid, "05-GraphicPipline");
}

void DepthBufferApp::SetupDescriptorPool() {
  // Example uses one ubo and one image sampler
  std::vector<VkDescriptorPoolSize> poolSizes = {
      initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
      initializers::DescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1)};

  VkDescriptorPoolCreateInfo descriptorPoolInfo =
      initializers::DescriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), 2);

  VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
}

void DepthBufferApp::SetupDescriptorSet() {
  VkDescriptorSetAllocateInfo allocInfo =
      initializers::DescriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);

  VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

  // Setup a descriptor image info for the current texture to be used as a
  // combined image sampler
  VkDescriptorImageInfo textureDescriptor = texture_->GetDescriptorImageInfo();

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      // Binding 0 : Vertex shader uniform buffer
      initializers::WriteDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                       &uniformBufferVS.descriptor_),
      // Binding 1 : Fragment shader texture sampler
      //	Fragment shader: layout (binding = 1) uniform sampler2D
      // samplerColor;
      initializers::WriteDescriptorSet(descriptorSet,
                                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  // The descriptor set will
                                                                                   // use a combined image
                                                                                   // sampler (sampler and
                                                                                   // image could be split)
                                       1,                                          // Shader binding point 1
                                       &textureDescriptor)  // Pointer to the descriptor image for our
                                                            // texture
  };

  vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0,
                         NULL);
}

void DepthBufferApp::BuildCommandBuffers() {
  VkCommandBufferBeginInfo cmdBufInfo = initializers::CommandBufferBeginInfo();

  VkClearValue clearValues[2];
  clearValues[0].color = defaultClearColor;
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo renderPassBeginInfo = initializers::RenderPassBeginInfo();
  renderPassBeginInfo.renderPass = renderPass;
  renderPassBeginInfo.renderArea.offset.x = 0;
  renderPassBeginInfo.renderArea.offset.y = 0;
  renderPassBeginInfo.renderArea.extent.width = width;
  renderPassBeginInfo.renderArea.extent.height = height;
  renderPassBeginInfo.clearValueCount = 2;
  renderPassBeginInfo.pClearValues = clearValues;

  for (int32_t i = 0; i < drawCmdBuffers.size(); ++i) {
    // Set target frame buffer
    renderPassBeginInfo.framebuffer = frameBuffers[i];

    VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

    vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = initializers::Viewport((float)width, (float)height, 0.0f, 1.0f);
    vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

    VkRect2D scissor = initializers::Rect2D(width, height, 0, 0);
    vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

    vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0,
                            NULL);
    vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.solid);

    VkDeviceSize offsets[1] = {0};
    auto vkvb = QuadMesh::instance()->getVertexVkBuffer();
    auto vkib = QuadMesh::instance()->getIndexVkBuffer();
    vkCmdBindVertexBuffers(drawCmdBuffers[i], VERTEX_BUFFER_BIND_ID, 1, &vkvb, offsets);
    vkCmdBindIndexBuffer(drawCmdBuffers[i], vkib, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(drawCmdBuffers[i], QuadMesh::instance()->IndexCount(), 1, 0, 0, 0);

    // drawUI(drawCmdBuffers[i]);

    vkCmdEndRenderPass(drawCmdBuffers[i]);

    VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
  }
}

void DepthBufferApp::LoadTexture() {
  texture_ = new VulkanTexture(vulkanDevice, "../../assets/texture.jpg", queue);
  texture_->LoadTexture();
  return;
}

void DepthBufferApp::Prepare() {
  VulkanApp::Prepare();
  LoadTexture();
  GenerateQuad();
  PrepareUniformBuffers();
  SetupDescriptorSetLayout();
  PreparePipelines();
  SetupDescriptorPool();
  SetupDescriptorSet();
  BuildCommandBuffers();
  prepared = true;
}

void DepthBufferApp::Draw() {
  VulkanApp::PrepareFrame();

  // Command buffer to be submitted to the queue
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

  // Submit to queue
  VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

  VulkanApp::SubmitFrame();
}

void DepthBufferApp::Render() {
  if (!prepared) return;
  Draw();
}
}  // namespace lvk

FILE *g_ic_file_cout_stream;
FILE *g_ic_file_cin_stream;

// Success: true , Failure: false
bool InitConsole() {
  if (!AllocConsole()) {
    return false;
  }
  if (freopen_s(&g_ic_file_cout_stream, "CONOUT$", "w", stdout) != 0) {
    return false;
  }  // For std::cout
  if (freopen_s(&g_ic_file_cin_stream, "CONIN$", "w+", stdin) != 0) {
    return false;
  }  // For std::cin
  return true;
}

using lvk::DepthBufferApp;
using lvk::VulkanApp;

static VulkanApp *vulkanApp{nullptr};
// Windows entry point
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  if (vulkanApp != NULL) {
    vulkanApp->HandleMessages(hWnd, uMsg, wParam, lParam);
  }
  return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
  InitConsole();
  for (int32_t i = 0; i < __argc; i++) {
    VulkanApp::args.push_back(__argv[i]);
  };
  vulkanApp = new DepthBufferApp();
  vulkanApp->InitVulkan();
  vulkanApp->SetupWindow(hInstance, WndProc);
  vulkanApp->Prepare();
  vulkanApp->RenderLoop();
  delete (vulkanApp);
  return 0;
}
