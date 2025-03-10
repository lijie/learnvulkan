#include "mesh_loader.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "lvk_log.h"
#include "lvk_math.h"
#include "primitives.h"


#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace lvk {

static size_t GetMeshPrimitivSize(const tinygltf::Mesh& mesh) {
#if 1
  return mesh.primitives.size();
#else
  return 1;
#endif
}

static void CopyToPrimitiveMeshIndices(lvk::PrimitiveMesh* lvk_mesh, const tinygltf::Model& model,
                                       const tinygltf::Mesh& mesh) {
  for (size_t i = 0; i < GetMeshPrimitivSize(mesh); ++i) {
    tinygltf::Primitive primitive = mesh.primitives[i];
    tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

    const tinygltf::BufferView& bufferView = model.bufferViews[indexAccessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    int byteStride = indexAccessor.ByteStride(model.bufferViews[indexAccessor.bufferView]);

    MeshSection* section = &lvk_mesh->sections[i];

    section->indices.resize(indexAccessor.count);
    for (int j = 0; j < section->indices.size(); j++) {
      memcpy(&section->indices[j], &buffer.data.at(0) + bufferView.byteOffset + j * byteStride, byteStride);
    }
  }
}

static void CopyToPrimitiveMeshAttribute(lvk::PrimitiveMesh* lvk_mesh, const tinygltf::Model& model,
                                         const tinygltf::Mesh& mesh) {
  for (size_t i = 0; i < GetMeshPrimitivSize(mesh); ++i) {
    const tinygltf::Primitive& primitive = mesh.primitives[i];
    MeshSection *section = &lvk_mesh->sections[i];

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
        if (section->vertices.size() < accessor.count) {
          section->vertices.resize(accessor.count);
          // lvk_mesh->indices.resize(accessor.count);
        }
        if (vaa == 0) {
          for (auto j = 0; j < section->vertices.size(); j++) {
            memcpy(&section->vertices[j].position, &buffer.data.at(0) + bufferView.byteOffset + byteStride * j,
                   byteStride);
          }
        } else if (vaa == 1) {
          for (auto j = 0; j < section->vertices.size(); j++) {
            memcpy(&section->vertices[j].normal, &buffer.data.at(0) + bufferView.byteOffset + byteStride * j,
                   byteStride);
          }
        } else if (vaa == 2) {
          for (auto j = 0; j < section->vertices.size(); j++) {
            memcpy(&section->vertices[j].uv, &buffer.data.at(0) + bufferView.byteOffset + byteStride * j, byteStride);
          }
        }
      } else
        std::cout << "vaa missing: " << attrib.first << std::endl;
    }
  }
}

static LoadMeshNode LoadGltfMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh) {
  LoadMeshNode result;
  result.MeshData = new PrimitiveMesh(GetMeshPrimitivSize(mesh));

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

  CopyToPrimitiveMeshIndices(result.MeshData, model, mesh);
  CopyToPrimitiveMeshAttribute(result.MeshData, model, mesh);

  for (size_t i = 0; i < GetMeshPrimitivSize(mesh); ++i) {
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

static LoadMeshNode LoadGltfSceneNode(const tinygltf::Model& model, const tinygltf::Node& node) {
  LoadMeshNode load_node;
  if ((node.mesh >= 0) && (node.mesh < model.meshes.size())) {
    load_node = LoadGltfMesh(model, model.meshes[node.mesh]);
  }

  for (size_t i = 0; i < node.children.size(); i++) {
    assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
    load_node = LoadGltfSceneNode(model, model.nodes[node.children[i]]);

    // TODO: only load one mesh? load multi mesh ?
    if (load_node.Valid()) {
      break;
    }
  }

  if (node.translation.size() >= 3) {
    load_node.transform.translation = vec3f(node.translation[0], node.translation[1], node.translation[2]);
  }

  if (node.scale.size() >= 3) {
    load_node.transform.scale = vec3f(node.scale[0], node.scale[1], node.scale[2]);
  }

  if (node.rotation.size() >= 4) {
    auto mat = matrix::MakeFromQuat(vec4f(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]));
    load_node.transform.rotation = matrix::DecomposeRotationFromMatrix(mat);
  }

  return load_node;
}

static LoadMeshResult LoadGltf(const std::string& path) {
  using namespace tinygltf;
  LoadMeshResult result;

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
    return result;
  }

  const tinygltf::Scene& scene = model.scenes[model.defaultScene];
  for (size_t i = 0; i < scene.nodes.size(); ++i) {
    assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
    auto load_node = LoadGltfSceneNode(model, model.nodes[scene.nodes[i]]);
    if (load_node.Valid()) {
      result.nodes.push_back(load_node);
    }
  }
  return result;
}

static bool IsGltfModel(const std::string_view path) { return true; }

LoadMeshResult MeshLoader::LoadMesh(const std::string& path) {
  if (!IsGltfModel(path)) {
    ERROR_LOG("invalid mesh type: {}", path);
    return LoadMeshResult();
  }
  return LoadGltf(path);
}
}  // namespace lvk