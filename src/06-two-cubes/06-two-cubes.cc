#include <array>
#include <iostream>

#include "base/scene.h"
#include "base/node.h"
#include "base/vulkan_app.h"
#include "base/vulkan_context.h"

namespace lvk {
using lvk::VulkanApp;

class TwoCubesApp : public VulkanApp {
 private:
  Scene scene;

  void InitScene();

 public:
  virtual void Render(double delta_time) override;
  virtual void Prepare() override;
};

void TwoCubesApp::InitScene() {
  // prepare resource
  scene.meshList = {
      primitive::cube(),
  };
  scene.textureList = {
      {"../assets/texture.jpg"},
  };
  scene.materialList = {
      {
          "06-vert.spv",
          "06-frag.spv",
      },
  };

  // init scene
  Node n1 = {.transform =
                 {
                     .translation{-1, 0, 0},
                     .rotation{0, 0, 0},
                     .scale{1, 1, 1},
                 },
             .mesh = 0,
             .material = 0,
             .materialParamters{
                 .textureList{0},
             }};

  Node n2 = {.transform =
                 {
                     .translation{+1, 0, 0},
                     .rotation{0, 0, 0},
                     .scale{1, 1, 1},
                 },
             .mesh = 0,
             .material = 0,
             .materialParamters{
                 .textureList{0},
             }};

  scene.AddNode(n1);
  scene.AddNode(n2);
}

void TwoCubesApp::Prepare() {
  VulkanApp::Prepare();
  InitScene();

  context_->CreateVulkanScene(&scene, vulkanDevice);
  context_->BuildCommandBuffers(&scene);
  prepared = true;
}

void TwoCubesApp::Render(double delta_time) {
  if (!prepared) return;
  context_->Draw(&scene);
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
