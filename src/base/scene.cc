#include "scene.h"

#include <memory>

#include "directional_light.h"
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

void Scene::AddNode(SNode node) {
  if (node->flags.EnableRender) {
    snodeList_.push_back(node);
  }

  if (node->node_type == NodeType::LightSource) {
    lightSourceList_.push_back(std::static_pointer_cast<LightSource>(node));
  }
}

void Scene::ForEachNode(std::function<void(Node *, int)> cb) {
  for (int i = 0; i < snodeList_.size(); i++) {
    cb(snodeList_[i].get(), i);
  }
}

Node *Scene::GetNode(int idx) { return snodeList_[idx].get(); }

const PrimitiveMesh *Scene::GetResourceMesh(int handle) { return &meshList[handle]; }
const Texture *Scene::GetResourceTexture(int handle) { return &textureList[handle]; }
const Material *Scene::GetResourceMaterial(int handle) { return &materialList[handle]; }

Scene::~Scene() {}
}  // namespace lvk