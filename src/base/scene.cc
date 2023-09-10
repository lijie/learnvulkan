#include "scene.h"

// #include <vcruntime.h>

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#include "glm/glm.hpp"

namespace lvk {

Scene::Scene() {
  cameraMatrix_.proj = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 10.0f);
  cameraMatrix_.view =
      glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

Node* Scene::AddNode(int meshIdx, int matIdx, const Transform& transform) {
  auto n = new Node{
      .transform = transform,
      .material = matIdx,
      .mesh = meshIdx,
  };
  nodeList_.push_back(n);
  return n;
}

void Scene::ForEachNode(std::function<void(Node*, int)> cb) {
  for (int i = 0; i < nodeList_.size(); i++) {
    cb(nodeList_[i], i);
  }
}

Node* Scene::GetNode(int idx) { return nodeList_[idx]; }

Scene::~Scene() {
  for (auto& n : nodeList_) {
    delete n;
  }
}
}  // namespace lvk