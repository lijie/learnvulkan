#include <array>
#include <iostream>

#include "base/scene.h"
#include "base/vulkan_app.h"
#include "base/vulkan_context.h"

namespace lvk {
using lvk::VulkanApp;

class TriangleApp : public VulkanApp {
 private:
  Scene scene;

  void InitScene();

 public:
  virtual void Render() override;
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
  Node n1 = {.transform =
                 {
                     .translation{0, 0, 0},
                     .rotation{0, 0, 0},
                     .scale{1, 1, 1},
                 },
             .mesh = 0,
             .material = 0,
             .materialParamters{
                 .textureList{0},
             }};

  scene.AddNode(n1);
}

void TriangleApp::Prepare() {
  VulkanApp::Prepare();
  InitScene();
  
  context_->CreateVulkanScene(&scene, vulkanDevice);
  context_->BuildCommandBuffers(&scene);
  prepared = true;
}

void TriangleApp::Render() {
  if (!prepared) return;
    context_->Draw();
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
