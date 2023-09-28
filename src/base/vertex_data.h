#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#include <array>
#include <vector>

#include "lvk_math.h"
#include "vulkan_buffer.h"

namespace lvk {

using Vector = glm::vec3;
using Vector3 = Vector;
using Vector2 = glm::vec2;

struct VertexLayout {
  Vector3 position;
  Vector3 normal;
  Vector2 uv;
};

}  // namespace lvk