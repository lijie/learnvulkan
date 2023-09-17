#include <array>
#include <iostream>

#include "base/scene.h"
#include "base/node.h"
#include "base/vulkan_app.h"
#include "base/vulkan_context.h"

namespace lvk {
using lvk::VulkanApp;

class MoveCameraApp : public VulkanApp {
 private:
  Scene scene;

  void InitScene();
  void Update(double deltaTime);

 public:
  virtual void Render(double deltaTime) override;
  virtual void Prepare() override;
};

void MoveCameraApp::InitScene() {
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

void MoveCameraApp::Update(double deltaTime) {
  auto n1 = scene.GetNode(0);
  auto LastRotation = n1->Rotation();
  LastRotation.y += deltaTime * 10;
  n1->SetRotation(LastRotation);

  auto n2 = scene.GetNode(1);
  LastRotation = n2->Rotation();
  LastRotation.z += deltaTime * 10;
  n2->SetRotation(LastRotation);
}

void MoveCameraApp::Prepare() {
  VulkanApp::Prepare();
  InitScene();

  context_->CreateVulkanScene(&scene, vulkanDevice);
  context_->BuildCommandBuffers(&scene);
  prepared = true;
}

void MoveCameraApp::Render(double deltaTime) {
  if (!prepared) return;
  Update(deltaTime);
  context_->Draw(&scene);

  // std::cout << deltaTime << std::endl;
}
}  // namespace lvk

using lvk::MoveCameraApp;
using lvk::VulkanApp;

static VulkanApp *vulkanApp{nullptr};

int main() {
  for (int32_t i = 0; i < __argc; i++) {
    VulkanApp::args.push_back(__argv[i]);
  };
  vulkanApp = new MoveCameraApp();
  vulkanApp->InitVulkan();
  // vulkanApp->SetupWindow(hInstance, WndProc);
  vulkanApp->Prepare();
  vulkanApp->RenderLoop();
  delete (vulkanApp);
  return 0;
}
