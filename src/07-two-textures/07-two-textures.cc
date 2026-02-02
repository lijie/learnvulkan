#include <array>
#include <iostream>

#include "base/scene.h"
#include "base/node.h"
#include "base/vulkan_app.h"
#include "base/vulkan_context.h"

namespace lvk {
using lvk::VulkanApp;

class TwoTexturesApp : public VulkanApp {
 private:
  Scene scene;

  void InitScene() override;

 public:
  virtual void Render(double delta_time) override;
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
  auto n1 = NewNode<Node>();
  n1->transform = {
      .translation{-1, 0, 0},
      .rotation{0, 0, 0},
      .scale{1, 1, 1},
  };
  n1->mesh = 0;
  n1->material = 0;
  n1->materialParamters = {
      .textureList{0},
  };

  auto n2 = NewNode<Node>();
  n2->transform = {
      .translation{+1, 0, 0},
      .rotation{0, 0, 0},
      .scale{1, 1, 1},
  };
  n2->mesh = 0;
  n2->material = 0;
  n2->materialParamters = {
      .textureList{1},
  };

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

void TwoTexturesApp::Render(double delta_time) {
  if (!prepared) return;
  context_->Draw(&scene);
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
