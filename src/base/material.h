#pragma once

#include <map>
#include <string>
#include <vector>

#include "lvk_math.h"

namespace lvk {

struct Material {
  std::string vertShaderPath;
  std::string fragShaderPath;
};

struct MaterialParamters {
  // pbr basic paramters
  vec3f baseColor{1.0, 1.0, 1.0};
  float roughness{0};
  float metallic{0};
  std::map<std::string, float> scalarMap;
  std::map<std::string, vec3f> vectorMap;
  std::vector<int> textureList;
};
}  // namespace lvk