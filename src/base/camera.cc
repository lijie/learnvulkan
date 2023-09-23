#include "camera.h"

#include <glm/gtx/string_cast.hpp>

#include "lvk_log.h"
#include "math.h"

namespace lvk {

Camera::Camera() {
  vec3f location = vec3f(0, 0, -4);
  SetLocationAndRotation(location, vec3f(0, 0, 0));
  SetPespective(45, 1280.0 / 720.0, 0.1, 10);
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

  camera_materix_.proj = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);
  // camera_materix_.proj[1][1] *= -1;
  camera_materix_.view =
      // glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
      glm::lookAt(location, location + forward, vec3f(0.0f, 1.0f, 0.0f));
}
}  // namespace lvk