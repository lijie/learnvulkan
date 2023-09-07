#pragma once

#include <vector>

#include "primitives.h"

namespace lvk {

struct PrimitiveMesh;
struct Node;

class Scene {
 public:
  Node* AddNode(int meshIdx, int matIdx, const Transform& transform);

 protected:
  std::vector<Node*> nodeList_;
};

}  // namespace lvk