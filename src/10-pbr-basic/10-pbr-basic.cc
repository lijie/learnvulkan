#define NOMINMAX

#include "base/directional_light.h"
#include "base/mesh_loader.h"
#include "base/node.h"
#include "base/scene.h"
#include "base/vulkan_app.h"
#include "base/vulkan_context.h"

lvk::PrimitiveMesh test_lvk_mesh;

namespace lvk {
using lvk::VulkanApp;

class PbrBasicApp : public VulkanApp {
 private:
  virtual void InitScene() override;
};

void PbrBasicApp::InitScene() {
  auto cube_mesh = MeshLoader::LoadMesh("..\\assets\\models\\teapot.gltf");
  // prepare resource
  scene.meshList = {
      // primitive::cube(),
      *cube_mesh.get(),
  };
  scene.textureList = {
      {"../assets/texture.jpg"},
      {"../assets/mutou.png"},
  };
  scene.materialList = {
      {
          "10-pbr-basic.vert.spv",
          "10-pbr-basic.frag.spv",
      },
  };

  auto n1 = NewNode<Node>();
  n1->mesh = 0;
  n1->material = 0;
  n1->materialParamters = {
      .baseColor{1.0f, 0.765557f, 0.336057f},  // gold
      .roughness = 0.5f,
      .metallic = 1.0,
      .textureList{0},
  };
  scene.AddNode(n1);

  auto light = NewNode<DirectionalLight>(
      Transform{.translation{0.0, 0.0, -6.0}, .rotation{0, 0, 0}, .scale{1.0, 1.0, 1.0}}, vec3f{1.0, 1.0, 1.0});
  scene.AddNode(light);
}
}  // namespace lvk

using lvk::PbrBasicApp;
using lvk::VulkanApp;

static VulkanApp* vulkanApp{nullptr};

int main() {
  for (int32_t i = 0; i < __argc; i++) {
    VulkanApp::args.push_back(__argv[i]);
  };
  vulkanApp = new PbrBasicApp();
  vulkanApp->InitVulkan();
  vulkanApp->Prepare();
  vulkanApp->RenderLoop();
  delete (vulkanApp);
  return 0;
}
