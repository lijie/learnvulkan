#include "scene.h"

// #include <vcruntime.h>
#include "stb_image.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#include "glm/glm.hpp"

namespace lvk {

uint8_t *Texture::Load(int *w, int *h, int *ch) {
  data = stbi_load(path.c_str(), w, h, ch, STBI_rgb_alpha);
  return data;
}

void Texture::Free() { stbi_image_free(data); }

Scene::Scene() {
  cameraMatrix_.proj = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 10.0f);
  cameraMatrix_.view =
      glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

Node *Scene::AddNode(int meshIdx, int matIdx, int texId, const Transform &transform) {
  auto n = new Node{
      .transform = transform,
      .material = matIdx,
      .mesh = meshIdx,
      .texture = texId,
  };
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

Scene::~Scene() {
  for (auto &n : nodeList_) {
    delete n;
  }
}
}  // namespace lvk