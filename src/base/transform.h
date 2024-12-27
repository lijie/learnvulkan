#pragma once

#include "lvk_math.h"

namespace lvk {

struct Transform {
  vec3f translation{0};
  vec3f rotation{0};
  vec3f scale{1};

  static Transform Identity;
};
}  // namespace lvk
