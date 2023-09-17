#include "camera.h"

#include <glm/gtx/string_cast.hpp>

#include "lvk_log.h"
#include "math.h"

namespace lvk {
void Camera::UpdateMatrix() {
  vec3f location = vec3f(0, 0, 4);
  SetLocationAndRotation(location, vec3f(0, 0, 0));
  DEBUG_LOG("camera matrix: {}", glm::to_string(localMatrix()));
  vec3f forward = GetForwardVector();

  camera_materix_.proj = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 10.0f);
  camera_materix_.proj[1][1] *= -1;
  camera_materix_.view =
      // glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
      glm::lookAt(location, location + forward, vec3f(0.0f, 1.0f, 0.0f));
}
}  // namespace lvk