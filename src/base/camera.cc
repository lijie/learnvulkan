#include "camera.h"

#include "lvk_math.h"

#include "lvk_log.h"

namespace lvk {

Camera::Camera() {
  vec3f location = vec3f(0.0f, 0.0f, -6.0f);
  SetLocationAndRotation(location, vec3f(0.0f, 0.0f, 0.0f));
  SetPespective(60, 1280.0 / 720.0, 0.1, 256);
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

// vulkan 使用左手坐标系
// camera 看 +z 方向
void Camera::UpdateMatrix() {
  // vec3f location = vec3f(0, 0, -4);
  vec3f location = transform.translation;
  // SetLocationAndRotation(location, vec3f(0, 0, 0));
  // DEBUG_LOG("camera matrix: {}", glm::to_string(localMatrix()));
  vec3f forward = GetForwardVector();

  camera_materix_.proj = glm::perspective(glm::radians(fov_), aspect_ratio_, near_, far_);
  // camera_materix_.proj[1][1] *= -1;
  auto r = glm::lookAt(location, location + forward, vec3f(0.0f, 1.0f, 0.0f));
  auto t = glm::translate(glm::mat4(1.0f), -location);
  camera_materix_.view = r * t;
  // camera_materix_.view =
  //     // glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  //     glm::lookAt(location, location + forward, vec3f(0.0f, 1.0f, 0.0f));
}
}  // namespace lvk