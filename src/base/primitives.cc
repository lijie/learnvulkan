#include "primitives.h"

#include "vulkan_tools.h"

namespace lvk {

void PrimitiveMeshVK::CreateBuffer(const PrimitiveMesh *mesh, VulkanDevice *device) {
  if (vertexBuffer == nullptr) {
    vertexBuffer = new VulkanBuffer();

    uint32_t vsize = static_cast<uint32_t>(sizeof(VertexLayout) * mesh->vertices.size());
    const void *vdata = mesh->vertices.data();
    VK_CHECK_RESULT(device->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                         vertexBuffer, vsize, vdata));
  }

  if (indexBuffer == nullptr) {
    indexBuffer = new VulkanBuffer();

    uint32_t isize = static_cast<uint32_t>(sizeof(VertexLayout) * mesh->indices.size());
    const void *idata = mesh->indices.data();
    VK_CHECK_RESULT(device->CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                         indexBuffer, isize, idata));
  }
}

PrimitiveMeshVK::~PrimitiveMeshVK() {
  if (vertexBuffer) {
    vertexBuffer->Destroy();
    delete vertexBuffer;
    vertexBuffer = nullptr;
  }

  if (indexBuffer) {
    indexBuffer->Destroy();
    delete indexBuffer;
    indexBuffer = nullptr;
  }
}

namespace primitive {
PrimitiveMesh quad() {
  return PrimitiveMesh{{{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                        {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                        {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}},
                       {0, 1, 2, 2, 3, 0}};
}
}  // namespace primitive
}  // namespace lvk