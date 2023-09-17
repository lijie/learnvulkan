#include "scene.h"

// #include <vcruntime.h>
#include "lvk_math.h"
#include "node.h"
#include "stb_image.h"

namespace lvk {

uint8_t *Texture::Load(int *w, int *h, int *ch) {
  data = stbi_load(path.c_str(), w, h, ch, STBI_rgb_alpha);
  return data;
}

void Texture::Free() { stbi_image_free(data); }

Scene::Scene() { default_camera_.UpdateMatrix(); }

Node *Scene::AddNode(int meshIdx, int matIdx, const Transform &transform) {
  auto n = new Node{
      .transform = transform,
      .mesh = meshIdx,
      .material = matIdx,
  };
  nodeList_.push_back(n);
  return n;
}

Node *Scene::AddNode(const Node &from) {
  auto n = new Node(from);
  nodeList_.push_back(n);
  return n;
}

void Scene::ForEachNode(std::function<void(Node *, int)> cb) {
  for (int i = 0; i < nodeList_.size(); i++) {
    cb(nodeList_[i], i);
  }
}

Node *Scene::GetNode(int idx) { return nodeList_[idx]; }

const PrimitiveMesh *Scene::GetResourceMesh(int handle) { return &meshList[handle]; }
const Texture *Scene::GetResourceTexture(int handle) { return &textureList[handle]; }
const Material *Scene::GetResourceMaterial(int handle) { return &materialList[handle]; }

Scene::~Scene() {
  for (auto &n : nodeList_) {
    delete n;
  }
}
}  // namespace lvk