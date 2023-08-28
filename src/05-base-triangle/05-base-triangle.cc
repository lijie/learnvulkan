#include <iostream>

#include "base/vulkan_app.h"

using lvk::VulkanApp;

class TriangleApp : public VulkanApp {
 private:
 public:
  void Render() {}
  virtual void Prepare() override;
};

void TriangleApp::Prepare() {
    VulkanApp::Prepare();
    prepared = true;
}

static VulkanApp *vulkanApp{nullptr};
// Windows entry point
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  if (vulkanApp != NULL) {
    vulkanApp->HandleMessages(hWnd, uMsg, wParam, lParam);
  }
  return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
  for (int32_t i = 0; i < __argc; i++) {
    VulkanApp::args.push_back(__argv[i]);
  };
  vulkanApp = new TriangleApp();
  vulkanApp->InitVulkan();
  vulkanApp->SetupWindow(hInstance, WndProc);
  vulkanApp->Prepare();
  vulkanApp->RenderLoop();
  delete (vulkanApp);
  return 0;
}
