#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class DrawingTriangleApp {
 public:
  void run() {
    InitWindow();
    InitVulkan();
    MainLoop();
    Cleanup();
  }

 private:
  GLFWwindow *window_;

  void InitWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window_ = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  }

  void InitVulkan() {}

  void MainLoop() {
    while (!glfwWindowShouldClose(window_)) {
      glfwPollEvents();
    }
  }

  void Cleanup() {
    glfwDestroyWindow(window_);
    glfwTerminate();
  }
};

int main() {
  DrawingTriangleApp app;
  app.run();
  return 0;
}