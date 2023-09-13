#pragma once

#include <functional>
#include <map>
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

struct Material {
  std::string vertShaderPath;
  std::string fragShaderPath;
};

struct MaterialParamters {
  std::map<std::string, float> scalarMap;
  std::map<std::string, vec3f> vectorMap;
  std::vector<int> textureList;
};

// render node in scene
struct Node {
  Transform transform;
  int mesh{-1};
  int material{0};
  MaterialParamters materialParamters;
  mat4f matrix{1.0};

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
  Node* AddNode(int meshIdx, int matIdx, const Transform& transform);
  Node* AddNode(const Node& from);
  void ForEachNode(std::function<void(Node*, int)> cb);

  ~Scene();

  Node* GetNode(int idx);
  size_t GetNodeCount() { return nodeList_.size(); }
  const CameraMatrix& GetCameraMatrix() { return cameraMatrix_; }

  // todo: resource manager
  std::vector<PrimitiveMesh> meshList;
  std::vector<Texture> textureList;
  std::vector<Material> materialList;

  const PrimitiveMesh* GetResourceMesh(int handle);
  const Texture* GetResourceTexture(int handle);
  const Material* GetResourceMaterial(int handle);

 protected:
  std::vector<Node*> nodeList_;
  CameraMatrix cameraMatrix_;
};

}  // namespace lvk