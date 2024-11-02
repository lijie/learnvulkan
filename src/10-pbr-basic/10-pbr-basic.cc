#include "base/vertex_data.h"
#define NOMINMAX

#include <array>
#include <iostream>

#include "base/directional_light.h"
#include "base/lvk_log.h"
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
  void InitScene();

 public:
  virtual void Update(float deltaTime) override;
  virtual void Render(double deltaTime) override;
  virtual void Prepare() override;
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

void PbrBasicApp::Update(float deltaTime) {
  VulkanApp::Update(deltaTime);

  // auto n1 = scene.GetNode(0);
  // auto LastRotation = n1->Rotation();
  // LastRotation.y += deltaTime * 10;
  // n1->SetRotation(LastRotation);

  // auto n2 = scene.GetNode(1);
  // LastRotation = n2->Rotation();
  // LastRotation.z += deltaTime * 10;
  // n2->SetRotation(LastRotation);

#if 0
  auto camera = scene.GetCamera();
  auto location = camera->GetLocation();
  location.z -= 0.1 * deltaTime;
  camera->SetLocation(location);
#endif
}

void PbrBasicApp::Prepare() {
  VulkanApp::Prepare();
  InitScene();

  context_->CreateVulkanScene(&scene, vulkanDevice);
  context_->BuildCommandBuffers(&scene);
  prepared = true;
}

void PbrBasicApp::Render(double deltaTime) {
  if (!prepared) return;
  Update(deltaTime);
  context_->Draw(&scene);

  // std::cout << deltaTime << std::endl;
}
}  // namespace lvk

using lvk::PbrBasicApp;
using lvk::VulkanApp;

static VulkanApp* vulkanApp{nullptr};

int main() {
  // test_lvk_mesh = LoadGltf();
  // assert(freopen("stdout.txt", "w", stdout) != NULL);
  // assert(freopen("stderr.txt", "w", stderr) != NULL);

  for (int32_t i = 0; i < __argc; i++) {
    VulkanApp::args.push_back(__argv[i]);
  };
  vulkanApp = new PbrBasicApp();
  vulkanApp->InitVulkan();
  // vulkanApp->SetupWindow(hInstance, WndProc);
  vulkanApp->Prepare();
  vulkanApp->RenderLoop();
  delete (vulkanApp);
  return 0;
}
