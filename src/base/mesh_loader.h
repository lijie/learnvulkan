#pragma once

#include <string.h>
#include <memory>
#include "primitives.h"

namespace lvk {

class MeshLoader {
 public:
  static std::unique_ptr<PrimitiveMesh> LoadMesh(const std::string& path);
};
}  // namespace lvk