#include "window.h"


#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace lvk {

class GlfwWindow : public Window {
 public:
  GlfwWindow(int width, int height);
  virtual bool ShouldClose() override;
  virtual void PollEvents() override;
  virtual bool CreateWindowSurface(void *instance, void *surface) override;

 private:
  GLFWwindow *window_{nullptr};
};

static void FramebufferResizeCallback(GLFWwindow *window, int width, int height) {
  auto app = reinterpret_cast<GlfwWindow *>(glfwGetWindowUserPointer(window));
  // app->framebuffer_resized_ = true;
}

GlfwWindow::GlfwWindow(int width, int height) {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window_ = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, FramebufferResizeCallback);
}

bool GlfwWindow::ShouldClose() { return glfwWindowShouldClose(window_); }

void GlfwWindow::PollEvents() { glfwPollEvents(); }

bool GlfwWindow::CreateWindowSurface(void *instance, void *surface) {
  return glfwCreateWindowSurface((VkInstance)instance, window_, nullptr, (VkSurfaceKHR *)surface) == VK_SUCCESS;
}

// factory
Window *Window::NewWindow(WindowType type, int width, int height) {
  if (type == WindowType::Glfw) {
    return new GlfwWindow(width, height);
  }
  return nullptr;
}
}  // namespace lvk