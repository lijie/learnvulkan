#include "base/node.h"

#include "lvk_log.h"
#include "lvk_math.h"

namespace lvk {

Node::Node() {
  transform.scale = vec3f(1.0, 1.0, 1.0);
}

mat4f Node::localMatrix() {
  matrix = mat4f(1.0);
  matrix = matrix::Translate(mat4f{1.0}, transform.translation);
  auto scale_mat = matrix::Scale(mat4f{1.0}, transform.scale);
  rot_matrix = matrix::MakeFromEulerAngleYXZ(transform.rotation.y, transform.rotation.x, transform.rotation.z);
  matrix = matrix * rot_matrix * scale_mat;
  return matrix;
}

void Node::UpdateModelMatrixTranslation() {
  matrix = matrix::Translate(mat4f{1.0}, transform.translation);
  auto scale_mat = matrix::Scale(mat4f{1.0}, transform.scale);
  matrix = matrix * rot_matrix * scale_mat;
}

void Node::UpdateModelMatrixScale() {
  matrix = matrix::Translate(mat4f{1.0}, transform.translation);
  auto scale_mat = matrix::Scale(mat4f{1.0}, transform.scale);
  matrix = matrix * rot_matrix * scale_mat;
}

void Node::SetRotation(const vec3f& in) {
  transform.rotation = in;
  ClampRotation(transform.rotation.y);
  ClampRotation(transform.rotation.x);
  ClampRotation(transform.rotation.z);
  localMatrix();
}

void Node::SetLocation(const vec3f& in) {
  transform.translation = in;
  UpdateModelMatrixTranslation();
}

void Node::SetScale(const vec3f& scale) {
  transform.scale = scale;
  UpdateModelMatrixScale();
}

void Node::SetScale1D(float scale) {
  transform.scale = vec3f(scale, scale, scale);
  UpdateModelMatrixScale();
}

void Node::SetRotationMatrix(const mat4f& in_matrix) {
  matrix = matrix::Translate(mat4f{1.0}, transform.translation);

  transform.rotation = matrix::DecomposeRotationFromMatrix(in_matrix);

  matrix = matrix * in_matrix;
  rot_matrix = in_matrix;
}

std::string Node::GetLocalMatrixString() { return glm::to_string(matrix); }

}  // namespace lvk