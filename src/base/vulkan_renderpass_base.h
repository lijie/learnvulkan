#include <vector>
#include "lvk_math.h"
#include "vulkan_renderpass.h"
#include "vulkan_buffer.h"

namespace lvk {

class VulkanBasePass : public VulkanRenderPass {
 public:
  virtual void Prepare() override;

 private:
  struct FrameBufferAttachment {
    VkImage image;
    VkDeviceMemory mem;
    VkImageView view;
  } depthStencil_;

  struct _UBOShared {
    vec4f camera_position;
    vec4f light_direction;
    vec4f light_color;
    mat4f light_mvp;
  };

  // uniform buffer object of mvp matrix
  struct _UBOMesh {
    mat4f projection;
    mat4f view;
    mat4f model;
  };

  // pbr parameters
  struct _UBOFragment {
    vec4f color;
    float roughness;
    float metallic;
    float padding[2];
    float padding2[8];
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

  std::vector<VkFramebuffer> frameBuffers_;

  void SetupFrameBuffer();
  void SetupRenderPass();
  void PrepareUniformBuffers();
  void UpdateUniformBuffers();
  void UpdateVertexUniformBuffers();
  void UpdateFragmentUniformBuffers();
  void UpdateSharedUniformBuffers();
};

}  // namespace lvk