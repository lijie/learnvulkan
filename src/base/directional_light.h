#pragma once

#include "node.h"

namespace lvk {

class DirectionalLight : public Node {
 public:
  void set_color(vec3f color) { color_ = color; }
  vec3f color() { return color_; }

 protected:
  vec3f color_;
};

}  // namespace lvk