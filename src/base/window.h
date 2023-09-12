#pragma once

namespace lvk {

enum class WindowType {
  Glfw,
};

class Window {
 public:
  virtual ~Window() {}

  virtual bool ShouldClose() = 0;

  virtual void PollEvents() = 0;

  virtual bool CreateWindowSurface(void *instance, void *surface) = 0;

  static Window* NewWindow(WindowType type, int width, int height);
};
}  // namespace lvk