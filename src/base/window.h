#pragma once

#include <functional>

namespace lvk {

enum class WindowType {
  Glfw,
};

struct WindowEventCallback {
  std::function<void(int key, int action)> OnKey;
  std::function<void(double xpos, double ypos)> OnMouseMove;
  std::function<void(int button, int action , int mod)> OnMouseClick;
};

class Window {
 public:
  virtual ~Window() {}

  virtual bool ShouldClose() = 0;

  virtual void PollEvents() = 0;

  virtual bool CreateWindowSurface(void *instance, void *surface) = 0;

  void SetEventCallbacks(const WindowEventCallback& callbacks) {
    event_callbacks_ = callbacks;
  }

  static Window *NewWindow(WindowType type, int width, int height);

  WindowEventCallback event_callbacks_;
};
}  // namespace lvk