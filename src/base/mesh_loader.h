#pragma once

#include "primitives.h"

namespace lvk {

struct LoadMeshResult {
  PrimitiveMesh* MeshData{nullptr};
  Transform transform;

  bool Valid() {
    return MeshData != nullptr;
  }
};

class MeshLoader {
 public:
  static LoadMeshResult LoadMesh(const std::string& path);
};
}  // namespace lvk
