#pragma once

#include <memory>

#include "lvk_math.h"
#include "node.h"
#include "transform.h"

namespace lvk {

enum class LightType {
  Directional,
  Point,
};

class LightSource : public Node {
 public:
  LightSource(const Transform& transform, const vec3f& color) : Node(transform, NodeType::LightSource){};
  void set_color(vec3f color) { color_ = color; }
  vec3f color() { return color_; }

 protected:
  vec3f color_;
  LightType light_type_{LightType::Directional};
};

class DirectionalLight : public LightSource {
 public:
  DirectionalLight(const Transform& transform, const vec3f& color) : LightSource(transform, color) {
    set_color(color);
    flags.EnableRender = 0;
    light_type_ = LightType::Directional;
  }
};

}  // namespace lvk