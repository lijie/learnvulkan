#include "mesh_loader.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "lvk_log.h"


#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace lvk {

static void CopyToPrimitiveMeshIndices(lvk::PrimitiveMesh* lvk_mesh, const tinygltf::Model& model,
                                       const tinygltf::Mesh& mesh) {
  // only support one primitives now
  assert(mesh.primitives.size() == 1);
  tinygltf::Primitive primitive = mesh.primitives[0];
  tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

  const tinygltf::BufferView& bufferView = model.bufferViews[indexAccessor.bufferView];
  const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
  int byteStride = indexAccessor.ByteStride(model.bufferViews[indexAccessor.bufferView]);

  lvk_mesh->vertices.resize(indexAccessor.count);
  lvk_mesh->indices.resize(indexAccessor.count);
  for (int j = 0; j < lvk_mesh->indices.size(); j++) {
    memcpy(&lvk_mesh->indices[j], &buffer.data.at(0) + bufferView.byteOffset + j * byteStride, byteStride);
  }
}

static void CopyToPrimitiveMeshAttribute(lvk::PrimitiveMesh* lvk_mesh, const tinygltf::Model& model,
                                         const tinygltf::Mesh& mesh) {
  for (size_t i = 0; i < mesh.primitives.size(); ++i) {
    tinygltf::Primitive primitive = mesh.primitives[i];

    for (auto& attrib : primitive.attributes) {
      tinygltf::Accessor accessor = model.accessors[attrib.second];
      int byteStride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);

      const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
      const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

      int size = 1;
      if (accessor.type != TINYGLTF_TYPE_SCALAR) {
        size = accessor.type;
      }

      int vaa = -1;
      if (attrib.first.compare("POSITION") == 0) vaa = 0;
      if (attrib.first.compare("NORMAL") == 0) vaa = 1;
      if (attrib.first.compare("TEXCOORD_0") == 0) vaa = 2;
      if (vaa > -1) {
        DEBUG_LOG("found attribute: {}, type: {}", attrib.first, accessor.type);
        if (lvk_mesh->vertices.size() < accessor.count) {
          lvk_mesh->vertices.resize(accessor.count);
          // lvk_mesh->indices.resize(accessor.count);
        }
        if (vaa == 0) {
          for (auto j = 0; j < lvk_mesh->vertices.size(); j++) {
            memcpy(&lvk_mesh->vertices[j].position, &buffer.data.at(0) + bufferView.byteOffset + byteStride * j,
                   byteStride);
          }
        } else if (vaa == 1) {
          for (auto j = 0; j < lvk_mesh->vertices.size(); j++) {
            memcpy(&lvk_mesh->vertices[j].normal, &buffer.data.at(0) + bufferView.byteOffset + byteStride * j,
                   byteStride);
          }
        } else if (vaa == 2) {
          for (auto j = 0; j < lvk_mesh->vertices.size(); j++) {
            memcpy(&lvk_mesh->vertices[j].uv, &buffer.data.at(0) + bufferView.byteOffset + byteStride * j, byteStride);
          }
        }
      } else
        std::cout << "vaa missing: " << attrib.first << std::endl;
    }
  }
}

static std::unique_ptr<PrimitiveMesh> LoadGltfMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh) {
  auto result = std::make_unique<PrimitiveMesh>();

  for (size_t i = 0; i < model.bufferViews.size(); ++i) {
    const tinygltf::BufferView& bufferView = model.bufferViews[i];
    if (bufferView.target == 0) {  // TODO impl drawarrays
      std::cout << "WARN: bufferView.target is zero" << std::endl;
      continue;  // Unsupported bufferView.
                 /*
                   From spec2.0 readme:
                   https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
                            ... drawArrays function should be used with a count equal to
                   the count            property of any of the accessors referenced by the
                   attributes            property            (they are all equal for a given
                   primitive).
                 */
    }

    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    std::cout << "bufferview.target " << bufferView.target << std::endl;

    std::cout << "buffer.data.size = " << buffer.data.size() << ", bufferview.byteOffset = " << bufferView.byteOffset
              << std::endl;
  }

  CopyToPrimitiveMeshIndices(result.get(), model, mesh);
  CopyToPrimitiveMeshAttribute(result.get(), model, mesh);

  for (size_t i = 0; i < mesh.primitives.size(); ++i) {
    tinygltf::Primitive primitive = mesh.primitives[i];
    tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

    const tinygltf::BufferView& bufferView = model.bufferViews[indexAccessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

#if 0
    lvk_mesh.vertices.resize(indexAccessor.count);
    lvk_mesh.indices.resize(indexAccessor.count);
    for (int j = 0; j < lvk_mesh.indices.size(); j++) {
      memcpy(&lvk_mesh.indices[j], &buffer.data.at(0) + bufferView.byteOffset + j * sizeof(uint16_t), sizeof(uint16_t));
    }
#endif

    for (auto& attrib : primitive.attributes) {
      tinygltf::Accessor accessor = model.accessors[attrib.second];
      int byteStride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);

      const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
      const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

      int size = 1;
      if (accessor.type != TINYGLTF_TYPE_SCALAR) {
        size = accessor.type;
      }

      int vaa = -1;
      if (attrib.first.compare("POSITION") == 0) vaa = 0;
      if (attrib.first.compare("NORMAL") == 0) vaa = 1;
      if (attrib.first.compare("TEXCOORD_0") == 0) vaa = 2;
      if (vaa > -1) {
#if 0
        DEBUG_LOG("found attribute: {}, type: {}", attrib.first, accessor.type);
        if (lvk_mesh.vertices.size() != accessor.count) {
          lvk_mesh.vertices.resize(accessor.count);
          lvk_mesh.indices.resize(accessor.count);
        }
        if (vaa == 0) {
          for (auto j = 0; j < lvk_mesh.vertices.size(); j++) {
            memcpy(&lvk_mesh.vertices[j].position, &buffer.data.at(0) + bufferView.byteOffset + byteStride * j,
                   byteStride);
          }
        } else if (vaa == 1) {
          for (auto j = 0; j < lvk_mesh.vertices.size(); j++) {
            memcpy(&lvk_mesh.vertices[j].normal, &buffer.data.at(0) + bufferView.byteOffset + byteStride * j,
                   byteStride);
          }
        } else if (vaa == 2) {
          for (auto j = 0; j < lvk_mesh.vertices.size(); j++) {
            memcpy(&lvk_mesh.vertices[j].uv, &buffer.data.at(0) + bufferView.byteOffset + byteStride * j, byteStride);
          }
        }
#endif
      } else
        std::cout << "vaa missing: " << attrib.first << std::endl;
    }
  }

  return result;
}

static std::unique_ptr<PrimitiveMesh> LoadGltfSceneNode(const tinygltf::Model& model, const tinygltf::Node& node) {
  std::unique_ptr<PrimitiveMesh> result;
  if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
    result = LoadGltfMesh(model, model.meshes[node.mesh]);
  }

  for (size_t i = 0; i < node.children.size(); i++) {
    assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
    result = LoadGltfSceneNode(model, model.nodes[node.children[i]]);
  }

  return result;
}

static std::unique_ptr<PrimitiveMesh> LoadGltf(const std::string& path) {
  using namespace tinygltf;

  Model model;
  TinyGLTF loader;
  std::string err;
  std::string warn;

  // bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, "..\\assets\\models\\Cube\\Cube.gltf");
  // bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, argv[1]); // for binary glTF(.glb)
  bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

  if (!warn.empty()) {
    printf("Warn: %s\n", warn.c_str());
  }

  if (!err.empty()) {
    printf("Err: %s\n", err.c_str());
  }

  if (!ret) {
    printf("Failed to parse glTF\n");
    return nullptr;
  }

  const tinygltf::Scene& scene = model.scenes[model.defaultScene];
  for (size_t i = 0; i < scene.nodes.size(); ++i) {
    assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
    return LoadGltfSceneNode(model, model.nodes[scene.nodes[i]]);
  }
  return nullptr;
}

static bool IsGltfModel(const std::string_view path) { return true; }

std::unique_ptr<PrimitiveMesh> MeshLoader::LoadMesh(const std::string& path) {
  if (!IsGltfModel(path)) {
    ERROR_LOG("invalid mesh type: {}", path);
    return nullptr;
  }
  return LoadGltf(path);
}
}  // namespace lvk