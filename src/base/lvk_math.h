#pragma once

#define GLM_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/gtx/string_cast.hpp>

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/glm.hpp"


namespace lvk {

using vec3f = glm::vec3;
using vec2f = glm::vec2;
using mat4f = glm::mat4x4;

constexpr vec3f ZERO_VECTOR = vec3f(0.0, 0.0, 0.0);

namespace vector {
constexpr vec3f UP = vec3f(0.0, 1.0, 0.0);
constexpr vec3f RIGHT = vec3f(1.0, 0.0, 0.0);
constexpr vec3f FORWARD = vec3f(0.0, 0.0, 1.0);
}  // namespace vector

}  // namespace lvk
