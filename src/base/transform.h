#pragma once

#include "lvk_math.h"

namespace lvk {

struct Transform {
  vec3f translation;
  vec3f rotation;
  vec3f scale;

  static Transform Identity;
};
}  // namespace lvk
