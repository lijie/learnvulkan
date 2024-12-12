#define NOMINMAX
#include "base/scene.h"
#include "base/node.h"
#include "base/directional_light.h"
#include "base/vulkan_app.h"
#include "base/vulkan_context.h"
#include "base/mesh_loader.h"
#include "base/vulkan_tools.h"

namespace lvk {
using lvk::VulkanApp;

class TwoCubesApp : public VulkanApp {
 protected:
  virtual void InitScene() override;

};

void TwoCubesApp::InitScene() {
  // auto cube_mesh = MeshLoader::LoadMesh(tools::GetModelPath() + "teapot.gltf");
  // prepare resource
  scene.meshList = {
      primitive::cube(),
  };
  scene.textureList = {
      {"../assets/texture.jpg"},
  };
  scene.materialList = {
      {
          "06-two-cubes.vert.spv",
          "06-two-cubes.frag.spv",
      },
  };


  // init scene
  auto n1 = NewNode<Node>();
  n1->SetTransform({
      .translation{-1, 0, 0},
      .rotation{45, 0, 0},
      .scale{1, 1, 1},
  });
  n1->mesh = 0;
  n1->material = 0;
  n1->materialParamters = {
      .textureList{0},
  };

  auto n2 = NewNode<Node>();
  n2->SetTransform({
      .translation{+1, 0, 0},
      .rotation{0, 0, 0},
      .scale{1, 1, 1},
  });
  n2->mesh = 0;
  n2->material = 0;
  n2->materialParamters = {
      .textureList{0},
  };

  scene.AddNode(n1);
  scene.AddNode(n2);

  auto light = NewNode<DirectionalLight>(
      Transform{.translation{0.0, 0.0, -6.0}, .rotation{0, 0, 0}, .scale{1.0, 1.0, 1.0}}, vec3f{1.0, 1.0, 1.0});
  scene.AddNode(light);
}

}  // namespace lvk

using lvk::TwoCubesApp;
using lvk::VulkanApp;

static VulkanApp *vulkanApp{nullptr};

int main() {
  for (int32_t i = 0; i < __argc; i++) {
    VulkanApp::args.push_back(__argv[i]);
  };
  vulkanApp = new TwoCubesApp();
  vulkanApp->InitVulkan();
  // vulkanApp->SetupWindow(hInstance, WndProc);
  vulkanApp->Prepare();
  vulkanApp->RenderLoop();
  delete (vulkanApp);
  return 0;
}
