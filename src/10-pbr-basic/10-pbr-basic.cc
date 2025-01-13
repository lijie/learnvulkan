#include <glm/ext/matrix_float4x4.hpp>
#include <glm/trigonometric.hpp>
#include "base/lvk_math.h"
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

#include <glm/gtx/euler_angles.hpp>

lvk::PrimitiveMesh test_lvk_mesh;

namespace lvk {
using lvk::VulkanApp;

class PbrBasicApp : public VulkanApp {
 private:
  void TestNode();
  virtual void InitScene() override;
  virtual void SetupUI(VulkanUI* ui) override;

  int model_index = 0;
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

  vec3f rot(45.0, 45.0, 45.0);
  vec3f translate(5.0, 6.0, 7.0);

  auto rm = glm::eulerAngleYXZ(glm::radians(rot.y), glm::radians(rot.x), glm::radians(rot.z));
  DEBUG_LOG("TestNode,eulerAngleYXZ,Matrix:\n{}", glm::to_string(rm));

  mat4f rot_mat{1.0};
  rot_mat = glm::rotate(rot_mat, glm::radians(rot.y), vec3f(0.0, 1.0, 0.0));
  rot_mat = glm::rotate(rot_mat, glm::radians(rot.x), vec3f(1.0, 0.0, 0.0));
  rot_mat = glm::rotate(rot_mat, glm::radians(rot.z), vec3f(0.0, 0.0, 1.0));
  DEBUG_LOG("TestNode,rotateYXZ,Matrix:\n{}", glm::to_string(rot_mat));

  // eulerAngleYXZ 等价于 YXZ 顺序各旋转一次
  assert(rm == rot_mat);

  // 旋转顺序不同, 旋转矩阵不同
  mat4f rot_mat2{1.0};
  rot_mat2 = glm::rotate(rot_mat2, glm::radians(rot.x), vec3f(1.0, 0.0, 0.0));
  rot_mat2 = glm::rotate(rot_mat2, glm::radians(rot.y), vec3f(0.0, 1.0, 0.0));
  rot_mat2 = glm::rotate(rot_mat2, glm::radians(rot.z), vec3f(0.0, 0.0, 1.0));
  DEBUG_LOG("TestNode,rotateXYZ,Matrix:\n{}", glm::to_string(rot_mat2));

  // translate 顺序不同, 矩阵不同
  // 原则上, translate 应该在最后 apply
  auto tra_mat = glm::translate(glm::mat4{1.0}, translate);
  DEBUG_LOG("TestNode,rotate before translate,Matrix:\n{}", glm::to_string(tra_mat * rot_mat));
  DEBUG_LOG("TestNode,rotate after translate,Matrix:\n{}", glm::to_string(rot_mat * tra_mat));
}

void PbrBasicApp::InitScene() {

  TestNode();

  // auto cube_mesh = MeshLoader::LoadMesh("..\\assets\\models\\teapot.gltf");
  // auto cube_mesh = MeshLoader::LoadMesh(tools::GetModelPath() + "teapot.gltf");
  // auto cube_mesh = MeshLoader::LoadMesh(tools::GetModelPath() + "Sponza\\glTF\\Sponza.gltf");
  // auto cube_mesh = MeshLoader::LoadMesh(tools::GetModelPath() + "cornell.gltf");
  // auto cube_mesh = MeshLoader::LoadMesh(tools::GetModelPath() + "sphere.gltf");
  auto cube_mesh = MeshLoader::LoadMesh(tools::GetModelPath() + "vulkanscene_shadow.gltf");
  
  assert(cube_mesh.Valid());

  for (const auto& load_node : cube_mesh.nodes) {
    scene.meshList.push_back(*load_node.MeshData);

    size_t index = scene.meshList.size() - 1;
    auto n1 = NewNode<Node>();
    n1->mesh = index;
    n1->material = 0;
    n1->materialParamters = {
        .baseColor{1.0f, 0.765557f, 0.336057f},  // gold
        .roughness = 0.5f,
        .metallic = 1.0,
        .textureList{0},
    };
    // n1->SetRotationMatrix(matrix::MakeFromQuat(vec4f(0.7071068286895752, 0, 0, 0.7071068286895752)));
    n1->SetTransform(cube_mesh.nodes[index].transform);
    // n1->SetScale1D(0.025);
    scene.AddNode(n1); 
  }

  // prepare resource
  // scene.meshList = {
  //     // primitive::cube(),
  //     *(cube_mesh.nodes[0].MeshData),
  // };
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


  auto light = NewNode<DirectionalLight>(
      Transform{.translation{0.0, 10.0, 0.0}, .rotation{0, 0, 0}, .scale{1.0, 1.0, 1.0}}, vec3f{1.0, 1.0, 1.0});
  scene.AddNode(light);
}

void PbrBasicApp::SetupUI(VulkanUI *ui) {
  if (ui->header("Settings")) {
    std::vector items = {std::string("aaa"), std::string("bbb")};
    if (ui->comboBox("Material", &model_index, items)) {
      DEBUG_LOG("Matrial: {}", model_index);
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
  vulkanApp->StartRenderdoc();
  vulkanApp->InitVulkan();
  vulkanApp->Prepare();
  vulkanApp->RenderLoop();
  vulkanApp->EndRenderdoc();
  delete (vulkanApp);

  
  return 0;
}
