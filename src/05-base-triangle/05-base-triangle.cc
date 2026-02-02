#include <array>
#include <iostream>

#include "base/scene.h"
#include "base/node.h"
#include "base/vulkan_app.h"
#include "base/vulkan_context.h"

namespace lvk {
using lvk::VulkanApp;

class TriangleApp : public VulkanApp {
 private:
  Scene scene;

  void InitScene() override;

 public:
  virtual void Render(double delta_time) override;
  virtual void Prepare() override;
};

void TriangleApp::InitScene() {
  // prepare resource
  scene.meshList = {
      primitive::quad(),
  };
  scene.textureList = {
      {"../assets/texture.jpg"},
  };
  scene.materialList = {
      {
          "05-vert.spv",
          "05-frag.spv",
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

  scene.AddNode(n1);
}

void TriangleApp::Prepare() {
  VulkanApp::Prepare();
  InitScene();
  
  context_->CreateVulkanScene(&scene, vulkanDevice);
  context_->BuildCommandBuffers(&scene);
  prepared = true;
}

void TriangleApp::Render(double delta_time) {
  if (!prepared) return;
    context_->Draw(&scene);
}
}  // namespace lvk

using lvk::TriangleApp;
using lvk::VulkanApp;

static VulkanApp *vulkanApp{nullptr};

int main() {
  for (int32_t i = 0; i < __argc; i++) {
    VulkanApp::args.push_back(__argv[i]);
  };
  vulkanApp = new TriangleApp();
  vulkanApp->InitVulkan();
  // vulkanApp->SetupWindow(hInstance, WndProc);
  vulkanApp->Prepare();
  vulkanApp->RenderLoop();
  delete (vulkanApp);
  return 0;
}
