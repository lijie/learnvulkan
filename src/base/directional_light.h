#pragma once

#include <memory>
#include "lvk_math.h"
#include "node.h"

namespace lvk {

class DirectionalLight : public Node {
 public:
  void set_color(vec3f color) { color_ = color; }
  vec3f color() { return color_; }

  static std::unique_ptr<DirectionalLight> NewDirectionalLight(Transform transform, vec3f color);

 protected:
  vec3f color_;
};

}  // namespace lvk