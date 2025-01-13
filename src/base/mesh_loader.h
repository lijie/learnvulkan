#pragma once

#include "primitives.h"

namespace lvk {

struct LoadMeshNode {
  PrimitiveMesh* MeshData{nullptr};
  Transform transform;

  bool Valid() { return MeshData != nullptr; }
};

struct LoadMeshResult {
  std::vector<LoadMeshNode> nodes;

  bool Valid() {
    return nodes.size() > 0;
  }
};

class MeshLoader {
 public:
  static LoadMeshResult LoadMesh(const std::string& path);
};
}  // namespace lvk
