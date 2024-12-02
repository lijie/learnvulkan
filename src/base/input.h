#pragma once

#include <vector>
#include <array>
#include "lvk_math.h"

namespace lvk {

enum class kInputDirection {
  Forward,
  Right,
};

enum class kInputKeyCode {
  W = 87,
  S = 83,
  A = 65,
  D = 68,

  UP = 265,
  DOWN = 264,
  LEFT = 263,
  RIGHT = 262,
};

struct DirectionInput {
  kInputDirection direction{kInputDirection::Forward};
  float scale{0.0};
};

struct InputHandle {
  int handle;
};

class InputComponent {
 public:
  InputComponent() {}

  // predefined input
  virtual void OnDirectionInput(const DirectionInput &di){};
  virtual void OnRotationInput(const DirectionInput &ri){};

  // general
  virtual void OnKey(int key, int action){};
  virtual void OnMouseClick(int button, int action){};
  virtual void OnMouseMove(double x, double y){};
};

class InputSystem {
 public:
  InputHandle AddInputComponent(InputComponent *ic);
  void RemoveInputComponent(InputHandle handle);

  // from window
  void OnKey(int key, int action);
  void OnMouseClick(int button, int action);
  void OnMouseMove(double x, double y);

  int GetMouseState(int button) {
    return mouse_state_[button];
  };

  vec2f GetMousePosition() {
    return mouse_position_;
  }

  void Update(float delta_time);

 private:
  std::vector<InputComponent *> ic_array_;
  std::array<int, 3> mouse_state_ = {0, 0, 0};
  vec2f mouse_position_;
};

}  // namespace lvk