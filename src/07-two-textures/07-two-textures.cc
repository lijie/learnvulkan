#include <array>
#include <iostream>

#include "base/scene.h"
#include "base/vulkan_app.h"
#include "base/vulkan_context.h"

namespace lvk {
using lvk::VulkanApp;

class TwoTexturesApp : public VulkanApp {
 private:
  Scene scene;

  void InitScene();

 public:
  virtual void Render() override;
  virtual void Prepare() override;
};

void TwoTexturesApp::InitScene() {
  // prepare resource
  scene.meshList = {
      primitive::cube(),
  };
  scene.textureList = {
      {"../assets/texture.jpg"},
      {"../assets/mutou.png"},
  };
  scene.materialList = {
      {
          "07-vert.spv",
          "07-frag.spv",
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
                 .textureList{1},
             }};

  scene.AddNode(n1);
  scene.AddNode(n2);
}

void TwoTexturesApp::Prepare() {
  VulkanApp::Prepare();
  InitScene();

  context_->CreateVulkanScene(&scene, vulkanDevice);
  context_->BuildCommandBuffers(&scene);
  prepared = true;
}

void TwoTexturesApp::Render() {
  if (!prepared) return;
  context_->Draw();
}
}  // namespace lvk

using lvk::TwoTexturesApp;
using lvk::VulkanApp;

static VulkanApp *vulkanApp{nullptr};

int main() {
  for (int32_t i = 0; i < __argc; i++) {
    VulkanApp::args.push_back(__argv[i]);
  };
  vulkanApp = new TwoTexturesApp();
  vulkanApp->InitVulkan();
  // vulkanApp->SetupWindow(hInstance, WndProc);
  vulkanApp->Prepare();
  vulkanApp->RenderLoop();
  delete (vulkanApp);
  return 0;
}
