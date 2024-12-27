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
  Line,
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

  NodeType node_type{NodeType::Object};
  NodeFlags flags;

  Node();
  Node(const Transform& in_transform, NodeType in_node_type) {
    transform = in_transform;
    node_type = in_node_type;
    localMatrix();
  }
  Node(const Transform& in_transform) : Node(in_transform, NodeType::Object) {}

  vec3f Rotation() const { return transform.rotation; }

  vec3f Translation() const { return transform.translation; }

  vec3f Scale() const { return transform.scale; }

  const mat4f& ModelMatrix() { return matrix; }

  inline void ClampRotation(float& x) {
    while (x > +360) x -= 360;
    while (x < -360) x += 360;
  }

  virtual void SetRotation(const vec3f& in);

  void SetLocation(const vec3f& in);

  void SetRotationMatrix(const mat4f& in_matrix);

  void SetLocationAndRotation(const vec3f& location, const vec3f rotation) {
    transform.translation = location;
    SetRotation(rotation);
    localMatrix();
  }

  void SetTransform(const Transform& in_transform) {
    transform = in_transform;
    localMatrix();
  }

  void SetScale(const vec3f& scale);

  vec3f GetLocation() { return transform.translation; }
  vec3f GetRotation() { return transform.rotation; }
  vec3f GetScale() { return transform.scale; }

  vec3f GetForwardVector() { return glm::normalize(vec3f(matrix[2][0], matrix[2][1], matrix[2][2])); }
  vec3f GetRightVector() {
    // TODO(andrewli): (-1) 是因为 flipY 了?
    return glm::normalize(vec3f(matrix[0][0], matrix[0][1], matrix[0][2])) * -1.0f;
  }
  std::string GetLocalMatrixString();

 private:
  mat4f localMatrix();
  void UpdateModelMatrixTranslation();
  void UpdateModelMatrixScale();

  // model matrix
  mat4f matrix{1.0};

  // rotation matrix
  mat4f rot_matrix{1.0};
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
