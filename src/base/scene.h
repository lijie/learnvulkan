#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "camera.h"
#include "material.h"
#include "primitives.h"

namespace lvk {

struct PrimitiveMesh;
struct Node;
class DirectionalLight;
class LightSource;

struct Texture {
  Texture() {}
  Texture(const std::string& inpath) { path = inpath; };

  std::string path;
  uint8_t* data{nullptr};

  uint8_t* Load(int* w, int* h, int* ch);
  void Free();
};

using SceneHandle = int;

struct Assets {};

class Scene {
 public:
  Scene();
  #if 0
  Node* AddNode(int meshIdx, int matIdx, const Transform& transform);
  Node* AddNode(const Node& from);
  #endif

  void AddNode(SNode node);

  void ForEachNode(std::function<void(Node*, int)> cb);

  void AddLight(std::unique_ptr<DirectionalLight> light);

  ~Scene();

  Node* GetNode(int idx);
  size_t GetNodeCount() { return snodeList_.size(); }
  const CameraMatrix& GetCameraMatrix() { return default_camera_.GetCameraMaterix(); }
  Camera* GetCamera() { return &default_camera_; }

  const std::vector<std::shared_ptr<LightSource>>& GetAllLights() { return lightSourceList_; };

  // todo: resource manager
  std::vector<PrimitiveMesh> meshList;
  std::vector<Texture> textureList;
  std::vector<Material> materialList;

  const PrimitiveMesh* GetResourceMesh(int handle);
  const Texture* GetResourceTexture(int handle);
  const Material* GetResourceMaterial(int handle);

 protected:
  std::vector<SNode> snodeList_;
  std::vector<std::shared_ptr<LightSource>> lightSourceList_;
  Camera default_camera_;
};

}  // namespace lvk