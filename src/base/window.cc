#include "window.h"

#include "lvk_log.h"

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

void key_callback(GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/) {
  DEBUG_LOG("input key code: {}, action: {}", key, action);
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
  DEBUG_LOG("mouse position: {}, {}", xpos, ypos);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int /*mods*/) {
  DEBUG_LOG("mouse button: {}, action: {}", button, action);
}

GlfwWindow::GlfwWindow(int width, int height) {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window_ = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, FramebufferResizeCallback);

  glfwSetKeyCallback(window_, key_callback);
  glfwSetCursorPosCallback(window_, cursor_position_callback);
  glfwSetMouseButtonCallback(window_, mouse_button_callback);

  glfwSetInputMode(window_, GLFW_STICKY_KEYS, 1);
  glfwSetInputMode(window_, GLFW_STICKY_MOUSE_BUTTONS, 1);
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