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
  std::map<std::string, float> scalarMap;
  std::map<std::string, vec3f> vectorMap;
  std::vector<int> textureList;
};
}  // namespace lvk