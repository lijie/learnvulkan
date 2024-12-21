#define NOMINMAX
#include "base/vulkan_app.h"

#include "base/directional_light.h"
#include "base/mesh_loader.h"
#include "base/node.h"
#include "base/scene.h"
#include "base/vulkan_context.h"
#include "base/vulkan_tools.h"
#include "base/lvk_log.h"
#include "base/vulkan_ui.h"

lvk::PrimitiveMesh test_lvk_mesh;

namespace lvk {
using lvk::VulkanApp;

class PbrBasicApp : public VulkanApp {
 private:
  void TestNode();
  virtual void InitScene() override;
  virtual void SetupUI(VulkanUI* ui) override;
};

void PbrBasicApp::TestNode() {
  auto n = NewNode<Node>(Transform{.translation{0.0, 0.0, 0}, .rotation{0, 0, 0}, .scale{1.0, 1.0, 1.0}});
  DEBUG_LOG("TestNode,InitMatrix,Matrix:\n{}", n->GetLocalMatrixString());

  n->SetRotation(vec3f(30.0f, 0.0f, 0.0f));
  DEBUG_LOG("TestNode,RotationX30,Matrix:\n{}", n->GetLocalMatrixString());

  n->SetRotation(vec3f(0.0f, 30.0f, 0.0f));
  DEBUG_LOG("TestNode,RotationY30,Matrix:\n{}", n->GetLocalMatrixString());

  n->SetRotation(vec3f(0.0f, 0.0f, 30.0f));
  DEBUG_LOG("TestNode,RotationZ30,Matrix:\n{}", n->GetLocalMatrixString());

  n->SetLocation(vec3f(2.0f, 2.0f, 2.0f));
  DEBUG_LOG("TestNode,AddTranslation,Matrix:\n{}", n->GetLocalMatrixString());

  n->SetRotation(vec3f(30.0f, 30.0f, 30.0f));
  DEBUG_LOG("TestNode,RotationXYZ30,Matrix:\n{}", n->GetLocalMatrixString());
}

void PbrBasicApp::InitScene() {

  TestNode();

  // auto cube_mesh = MeshLoader::LoadMesh("..\\assets\\models\\teapot.gltf");
  // auto cube_mesh = MeshLoader::LoadMesh(tools::GetModelPath() + "teapot.gltf");
  // auto cube_mesh = MeshLoader::LoadMesh(tools::GetModelPath() + "Sponza\\glTF\\Sponza.gltf");
  auto cube_mesh = MeshLoader::LoadMesh(tools::GetModelPath() + "cornell.gltf");
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

void PbrBasicApp::SetupUI(VulkanUI *ui) {
  if (ui->header("Settings")) {
    int index = 0;
    std::vector items = {std::string("aaa"), std::string("bbb")};
    if (ui->comboBox("Material", &index, items)) {
    }
  }
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
