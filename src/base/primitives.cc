#include "primitives.h"

#include "vertex_data.h"
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

    // TODO: bug? sizeof(uint32)?
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

static uint32_t addPos(PrimitiveMesh &mesh, vec3f p) {
  VertexLayout v{};
  v.position = p;
  mesh.vertices.emplace_back(v);
  return static_cast<uint32_t>(mesh.vertices.size()) - 1;
}

static void addTriangle(PrimitiveMesh &mesh, uint32_t a, uint32_t b, uint32_t c) {
  mesh.indices.push_back(a);
  mesh.indices.push_back(b);
  mesh.indices.push_back(c);
}

static void addTriangle(PrimitiveMesh &mesh, vec3f a, vec3f b, vec3f c) {
  mesh.indices.push_back(addPos(mesh, a));
  mesh.indices.push_back(addPos(mesh, b));
  mesh.indices.push_back(addPos(mesh, c));
}

PrimitiveMesh cube(float width /*= 1*/, float height /*= 1*/, float depth /*= 1*/) {
  PrimitiveMesh mesh;

  vec3f s = vec3f(width, height, depth) * 0.5F;
  std::vector<vec3f> pnt = {{-s.x, -s.y, -s.z}, {-s.x, -s.y, s.z}, {-s.x, s.y, -s.z}, {-s.x, s.y, s.z},
                            {s.x, -s.y, -s.z},  {s.x, -s.y, s.z},  {s.x, s.y, -s.z},  {s.x, s.y, s.z}};
  std::vector<vec3f> nrm = {{-1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F},  {1.0F, 0.0F, 0.0F},
                            {0.0F, 0.0F, -1.0F}, {0.0F, -1.0F, 0.0F}, {0.0F, 1.0F, 0.0F}};
  std::vector<vec2f> uv = {{0.0F, 0.0F}, {0.0F, 1.0F}, {1.0F, 1.0F}, {1.0F, 0.0F}};

  // cube topology
  std::vector<std::vector<int>> cube_polygons = {{0, 1, 3, 2}, {1, 5, 7, 3}, {5, 4, 6, 7},
                                                 {4, 0, 2, 6}, {4, 5, 1, 0}, {2, 3, 7, 6}};

  for (int i = 0; i < 6; ++i) {
    auto index = static_cast<int32_t>(mesh.vertices.size());
    for (int j = 0; j < 4; ++j) mesh.vertices.push_back({pnt[cube_polygons[i][j]], nrm[i], uv[j]});
    addTriangle(mesh, index, index + 1, index + 2);
    addTriangle(mesh, index, index + 2, index + 3);
  }

  return mesh;
}

PrimitiveMesh quad() {
  return PrimitiveMesh{{{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                        {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                        {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}},
                       {0, 1, 2, 2, 3, 0}};
}
}  // namespace primitive
}  // namespace lvk