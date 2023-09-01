#include <string>

#include "vulkan/vulkan.h"

namespace lvk {

class VulkanDevice;

class VulkanTexture {
 public:
  VulkanTexture(VulkanDevice *device, const std::string &path, VkQueue queue)
      : device_(device), path_(path), queue_(queue){};
  ~VulkanTexture(){};

  void LoadTexture();
  VkDescriptorImageInfo GetDescriptorImageInfo();

 private:
  VulkanDevice *device_{nullptr};
  std::string path_;
  uint32_t width_, height_;
  uint32_t mipLevels_;
  VkFormat format_{VK_FORMAT_R8G8B8A8_UNORM};
  VkImage image_;
  VkDeviceMemory deviceMemory_;
  VkImageView view_;
  VkImageLayout imageLayout_;
  VkSampler sampler_;
  // todo: remove
  VkQueue queue_{VK_NULL_HANDLE};
};

}  // namespace lvk
