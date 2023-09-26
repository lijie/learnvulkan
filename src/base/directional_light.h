#pragma once

#include "node.h"

namespace lvk {

class DirectionalLight : public Node {
 public:
 protected:
  vec3f color_;
};

}  // namespace lvk