#pragma once

#include <functional>
#include <vector>

#include "primitives.h"

namespace lvk {

struct PrimitiveMesh;
struct Node;

struct CameraMatrix {
  mat4f view{1};
  mat4f proj{1};
};

class Scene {
 public:
  Scene();
  Node* AddNode(int meshIdx, int matIdx, const Transform& transform);
  void ForEachNode(std::function<void(Node*, int)> cb);

  ~Scene();

  Node* GetNode(int idx);
  size_t GetNodeCount() { return nodeList_.size(); }
  const CameraMatrix& GetCameraMatrix() { return cameraMatrix_; }

 protected:
  std::vector<Node*> nodeList_;
  CameraMatrix cameraMatrix_;
};

}  // namespace lvk