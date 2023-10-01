#pragma once

#include <stdint.h>

#include <memory>

#include "lvk_math.h"
#include "material.h"
#include "transform.h"

namespace lvk {

enum class NodeType {
  Object,
  Camera,
  LightSource,
  Skybox,
  MAX,
};

struct NodeFlags {
  uint32_t EnableRender : 1 = 1;
};

// render node in scene
struct Node {
  Transform transform;
  int mesh{-1};
  int material{0};
  MaterialParamters materialParamters;
  mat4f matrix{1.0};

  NodeType node_type{NodeType::Object};
  NodeFlags flags;

  Node() {}
  Node(const Transform& in_transform, NodeType in_node_type) {
    transform = in_transform;
    node_type = in_node_type;
  }
  Node(const Transform& in_transform) : Node(in_transform, NodeType::Object) {}

  mat4f localMatrix() {
#if 0
    mat4f m{1.0};
    m = glm::scale(m, transform.scale);
    m = glm::rotate(m, transform.rotation.y, vec3f(0, 1, 0));
    m = glm::rotate(m, transform.rotation.x, vec3f(1, 0, 0));
    m = glm::rotate(m, transform.rotation.z, vec3f(0, 0, 1));
    m = glm::translate(m, transform.translation);
    // cache result
    matrix = m;
#endif
    matrix = glm::translate(mat4f{1.0}, transform.translation);
    matrix = glm::rotate(matrix, glm::radians(transform.rotation.x), vec3f(1, 0, 0));
    matrix = glm::rotate(matrix, glm::radians(transform.rotation.y), vec3f(0, 1, 0));
    matrix = glm::rotate(matrix, glm::radians(transform.rotation.z), vec3f(0, 0, 1));
    return matrix;
  }

  vec3f Rotation() const { return transform.rotation; }

  vec3f Translation() const { return transform.translation; }

  vec3f Scale() const { return transform.scale; }

  inline void ClampRotation(float& x) {
    if (x > +360) x -= 360;
    if (x < -360) x += 360;
  }

  void SetRotation(const vec3f& in) {
    transform.rotation = in;
    ClampRotation(transform.rotation.y);
    ClampRotation(transform.rotation.x);
    ClampRotation(transform.rotation.z);
  }

  void SetLocation(const vec3f& in) { transform.translation = in; }

  void SetLocationAndRotation(const vec3f& location, const vec3f rotation) {
    transform.translation = location;
    SetRotation(rotation);
  }

  vec3f GetLocation() { return transform.translation; }
  vec3f GetRotation() { return transform.rotation; }

  vec3f GetForwardVector() {
    localMatrix();
    return glm::normalize(vec3f(matrix[2][0], matrix[2][1], matrix[2][2]));
  }
  vec3f GetRightVector() {
    localMatrix();
    return glm::normalize(vec3f(matrix[0][0], matrix[0][1], matrix[0][2]));
  }
};

typedef std::shared_ptr<Node> SNode;

template <typename T, typename... TArgs>
static SNode NewNode(const Transform& transform = Transform::Identity, TArgs... args) {
  // ...
  // ...
  auto node = std::make_shared<T>(transform, args...);
  return node;
}

}  // namespace lvk
