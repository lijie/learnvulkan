#include "directional_light.h"

#include <memory>

namespace lvk {

std::unique_ptr<DirectionalLight> DirectionalLight::NewDirectionalLight(Transform transform, vec3f color) {
  auto light = std::make_unique<DirectionalLight>();
  light->transform = transform;
  light->set_color(color);
  return light;
}

}  // namespace lvk
