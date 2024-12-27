#include "camera.h"

#include "lvk_math.h"

#include "lvk_log.h"
#include "node.h"

namespace lvk {

// [DEBUG][lvk::DefaultCameraMoveInput::UpdateMouseLookMode] Camera location: vec3(0.000000, -4.457829, -4.015927)
// [DEBUG][lvk::DefaultCameraMoveInput::UpdateMouseLookMode] Camera rotation: vec3(-47.985245, 0.000000, 0.000000)

// [DEBUG][lvk::DefaultCameraMoveInput::UpdateMouseLookMode] Camera location: vec3(-0.755878, 0.646053, -0.734121)
// [DEBUG][lvk::DefaultCameraMoveInput::UpdateMouseLookMode] Camera rotation: vec3(31.512556, 45.836651, 0.000000)
Camera::Camera() {
  vec3f location = vec3f(0.0f, 0.0f, -6.0f);
  // SetLocationAndRotation(vec3f(0.000000, 3.135936, -5.114825), vec3f(31.512556, 0.000000, 0.000000));
  SetLocationAndRotation(vec3f(0.000000, 0, -6), vec3f(0, 0.000000, 0.000000));
  SetPespective(60, 1280.0 / 720.0, 0.1, 256);
  node_type = NodeType::Camera;
}

const CameraMatrix& Camera::GetCameraMaterix() {
  UpdateMatrix();
  return camera_materix_;
}

void Camera::SetPespective(float fov, float aspect_ratio, float near, float far) {
  fov_ = fov;
  aspect_ratio_ = aspect_ratio;
  near_ = near;
  far_ = far;
}

// vulkan 使用右手坐标系
// camera 看 +z 方向
void Camera::UpdateMatrix() {
  // vec3f location = vec3f(0, 0, -4);
  vec3f location = transform.translation;
  // SetLocationAndRotation(location, vec3f(0, 0, 0));
  // DEBUG_LOG("camera matrix: {}", GetLocalMatrixString());
  vec3f forward = GetForwardVector();

  camera_materix_.proj = glm::perspective(glm::radians(fov_), aspect_ratio_, near_, far_);
  // camera_materix_.proj[1][1] *= -1;
  auto r = glm::lookAt(location, location + forward, vec3f(0.0f, 1.0f, 0.0f));
  // auto r = glm::lookAt(location, vec3f(0, 0, 0), vec3f(0.0f, 1.0f, 0.0f));
  auto t = glm::translate(glm::mat4(1.0f), -location);
  camera_materix_.view = r * t;
  // camera_materix_.view =
  //     // glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  //     glm::lookAt(location, location + forward, vec3f(0.0f, 1.0f, 0.0f));
}

void Camera::SetRotation(const vec3f& in) {
  auto fix_rotation = in;
  ClampRotation(fix_rotation.x);

  if (fix_rotation.x <= -90.0f) {
    fix_rotation.x = -89.99;
  }
  if (fix_rotation.x >= 90.0f) {
    fix_rotation.x = 99.99;
  }
  Node::SetRotation(fix_rotation);
}

}  // namespace lvk