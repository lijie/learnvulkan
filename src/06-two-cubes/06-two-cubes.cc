#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include "base/lvk_log.h"
#include "base/lvk_math.h"
#define NOMINMAX
#include "base/scene.h"
#include "base/node.h"
#include "base/directional_light.h"
#include "base/vulkan_app.h"
#include "base/vulkan_context.h"
#include "base/mesh_loader.h"
#include "base/vulkan_tools.h"
#include <glm/gtx/rotate_vector.hpp>

namespace lvk {
using lvk::VulkanApp;

class TwoCubesApp : public VulkanApp {
 protected:
  virtual void InitScene() override;
  void Update(float delta_time) override;

  void RotateN1(SNode n1, float delta_time);
  void RotateN2(SNode n1, float delta_time);
  void ScaleN1(SNode n1, float delta_time);

  SNode n1;
  SNode n2;

  int n2_dir = 1;
  int n1_scale_dir = 1;
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

#if 0
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
    auto n1 = NewNode<Node>();
    n1->SetTransform({
        .translation{(-4 + i) * 1.5, (-4 + j) * 1.5, 0},
        .rotation{0, 0, 45},
        .scale{1, 1, 1},
    });
    n1->mesh = 0;
    n1->material = 0;
    n1->materialParamters = {
        .textureList{0},
    };
    scene.AddNode(n1);
    }
  }
#endif

    n1 = NewNode<Node>();
    n1->SetTransform({
        .translation{0, 0, 0},
        .rotation{0, 0, 0},
        .scale{1, 1, 1},
    });
    n1->mesh = 0;
    n1->material = 0;
    n1->materialParamters = {
        .textureList{0},
    };
    scene.AddNode(n1);
    DEBUG_LOG("n1, forward: {}", glm::to_string(n1->GetForwardVector()));

    n2 = NewNode<Node>();
    n2->SetTransform({
        .translation{3, 0, 0},
        .rotation{0, 0, 0},
        .scale{1, 1, 1},
    });
    n2->mesh = 0;
    n2->material = 0;
    n2->materialParamters = {
        .textureList{0},
    };
    scene.AddNode(n2);
    DEBUG_LOG("n2, forward: {}", glm::to_string(n1->GetForwardVector()));

  auto light = NewNode<DirectionalLight>(
      Transform{.translation{0.0, 0.0, -6.0}, .rotation{0, 0, 0}, .scale{1.0, 1.0, 1.0}}, vec3f{1.0, 1.0, 1.0});
  scene.AddNode(light);
}

void TwoCubesApp::RotateN1(SNode n1, float delta_time) {
  auto rotation = n1->GetRotation();
  rotation.z += 10 * delta_time;
  rotation.y += 10 * delta_time;
  rotation.x += 10 * delta_time;
  // n1->SetRotation(rotation);
}

void TwoCubesApp::RotateN2(SNode n1, float delta_time) {
  auto angle = delta_time * 2;

  auto new_location1 = glm::rotate(n1->GetLocation(), angle, vec3f(0, 1, 0));
  n1->SetLocation(new_location1);

  // auto new_location2 = glm::rotate(n1->GetLocation(), angle, vec3f(0, 0, 1));
  // n1->SetLocation(new_location2);

  // DEBUG_LOG("n2, forward: {}", glm::to_string(n1->GetForwardVector()));
  // DEBUG_LOG("n2, location normal: {}", glm::to_string(glm::normalize(n1->GetLocation()) * vec3f(-1, -1, -1)));

  // look at
  // 为什么要从 原点 看 Location 呢?, 反过来的话, 算出来的 forward 轴反了?
  // 为什么要 inverse 呢?
  auto mat = glm::inverse(glm::lookAt(vec3f(0, 0, 0), n1->GetLocation(), vec3f(0, 1, 0)));
  n1->SetRotationMatrix(mat);

  if (glm::length(n1->GetLocation()) < 0.5) {
    n2_dir = -1;
  }
  if (glm::length(n1->GetLocation()) > 5) {
    n2_dir = +1;
  }

#if 1
  auto forward = n1->GetForwardVector() * vec3f(n2_dir, n2_dir, n2_dir);
  auto next_location = n1->GetLocation() + (forward * delta_time * 0.2f);
  n1->SetLocation(next_location);
#endif
  // auto quat = glm::angleAxis(glm::radians(angle), glm::vec3(1, 0, 0));
  // n1->SetRotationMatrix(glm::mat4_cast(quat));
}

void TwoCubesApp::ScaleN1(SNode n1, float delta_time) {
  auto scale = n1->GetScale();
  scale += delta_time * 10 * n1_scale_dir;
  n1->SetScale(scale);

  if (scale.x > 10.0 || scale.x < -10.0) {
    n1_scale_dir *= -1;
  }
}

void TwoCubesApp::Update(float delta_time) {
  VulkanApp::Update(delta_time);
  RotateN1(n1, delta_time);
  RotateN2(n2, delta_time);
  ScaleN1(n1, delta_time);
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
