#pragma once

#include "node.h"

namespace lvk {

struct CameraMatrix {
  mat4f view{1};
  mat4f proj{1};
};

class Camera : public Node {
 public:
  Camera();
  const CameraMatrix& GetCameraMaterix();

  void SetPespective(float fov, float aspect_ratio, float near, float far);
  void UpdateMatrix();

 private:
  CameraMatrix camera_materix_;

  float fov_{45.0};
  float aspect_ratio_{1280.0 / 720.0};
  float near_{0.1};
  float far_{10.0};
};

}  // namespace lvk
