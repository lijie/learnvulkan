#include <string>

#include "vulkan/vulkan.h"

#include "lvk_math.h"

namespace lvk {
namespace debug {
// Default debug callback
VKAPI_ATTR VkBool32 VKAPI_CALL MessageCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType,
                                               uint64_t srcObject, size_t location, int32_t msgCode,
                                               const char* pLayerPrefix, const char* pMsg, void* pUserData);

// Load debug function pointers and set debug callback
void SetupDebugging(VkInstance instance);
// Clear debug callback
void FreeDebugCallback(VkInstance instance);
}  // namespace debug
namespace debugutils {
void Setup(VkInstance instance);
void CmdBeginLabel(VkCommandBuffer cmdbuffer, std::string caption, glm::vec4 color);
void CmdEndLabel(VkCommandBuffer cmdbuffer);

VkResult SetDebugObjectName(VkDevice device, VkObjectType type, uint64_t handle, const char* name);
}  // namespace debugutils
}  // namespace lvk