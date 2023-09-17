#pragma once

#include "node.h"

namespace lvk {

struct CameraMatrix {
  mat4f view{1};
  mat4f proj{1};
};

class Camera : public Node {
 public:
  const CameraMatrix& CameraMaterix() { return camera_materix_; }

  void SetParamters(float fov, float ratio, float near, float far);
  void UpdateMatrix();

 private:
  CameraMatrix camera_materix_;
};

}  // namespace lvk
