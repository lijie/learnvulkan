#pragma once

#include <functional>
#include <string>
#include <vector>

#include "primitives.h"

namespace lvk {

struct PrimitiveMesh;
struct Node;

struct CameraMatrix {
  mat4f view{1};
  mat4f proj{1};
};

struct Texture {
  Texture() {}
  Texture(const std::string& inpath) { path = inpath; };

  std::string path;
  uint8_t* data{nullptr};

  uint8_t* Load(int* w, int* h, int* ch);
  void Free();
};

using SceneHandle = int;

// render node in scene
struct Node {
  Transform transform;
  mat4f matrix{1.0};
  int material{0};
  int mesh{-1};
  int texture{0};

  mat4f localMatrix() {
    mat4f& m = matrix;
    m = glm::scale(m, transform.scale);
    m = glm::rotate(m, transform.rotation.y, vec3f(0, 1, 0));
    m = glm::rotate(m, transform.rotation.x, vec3f(1, 0, 0));
    m = glm::rotate(m, transform.rotation.z, vec3f(0, 0, 1));
    m = glm::translate(m, transform.translation);
    return m;
  }
};

struct Assets {};

class Scene {
 public:
  Scene();
  Node* AddNode(int meshIdx, int matIdx, int texId, const Transform& transform);
  void ForEachNode(std::function<void(Node*, int)> cb);

  ~Scene();

  Node* GetNode(int idx);
  size_t GetNodeCount() { return nodeList_.size(); }
  const CameraMatrix& GetCameraMatrix() { return cameraMatrix_; }

  // todo: resource manager
  std::vector<PrimitiveMesh> meshList;
  std::vector<Texture> textureList;

  const PrimitiveMesh* GetResourceMesh(int handle);
  const Texture* GetResourceTexture(int handle);

 protected:
  std::vector<Node*> nodeList_;
  CameraMatrix cameraMatrix_;
};

}  // namespace lvk