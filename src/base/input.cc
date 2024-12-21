#include "input.h"

#include "lvk_log.h"

namespace lvk {

InputHandle InputSystem::AddInputComponent(InputComponent* ic) {
  InputHandle handle;
  ic_array_.push_back(ic);
  handle.handle = ic_array_.size() - 1;
  return handle;
}

void InputSystem::RemoveInputComponent(InputHandle handle) {
  // just clear
  ic_array_[handle.handle] = nullptr;
}

void InputSystem::OnKey(int key, int action) {
  DEBUG_LOG("key:{}, action:{}", key, action);
  DirectionInput di;
  DirectionInput ri;
  if (action != 0) {
    if (static_cast<KeyCode>(key) == KeyCode::W) {
      di.direction = kInputDirection::Forward;
      di.scale = 1.0;
    } else if (static_cast<KeyCode>(key) == KeyCode::S) {
      di.direction = kInputDirection::Forward;
      di.scale = -1.0;
    } else if (static_cast<KeyCode>(key) == KeyCode::D) {
      di.direction = kInputDirection::Right;
      di.scale = -1.0;
    } else if (static_cast<KeyCode>(key) == KeyCode::A) {
      di.direction = kInputDirection::Right;
      di.scale = 1.0;
    } else if (static_cast<KeyCode>(key) == KeyCode::UP) {
      ri.direction = kInputDirection::Forward;
      ri.scale = 1.0;
    } else if (static_cast<KeyCode>(key) == KeyCode::DOWN) {
      ri.direction = kInputDirection::Forward;
      ri.scale = -1.0;
    } else if (static_cast<KeyCode>(key) == KeyCode::LEFT) {
      ri.direction = kInputDirection::Right;
      ri.scale = 1.0;
    } else if (static_cast<KeyCode>(key) == KeyCode::RIGHT) {
      ri.direction = kInputDirection::Right;
      ri.scale = -1.0;
    }
  }

  for (const auto& ic : ic_array_) {
    if (di.scale != 0.0) {
      ic->OnDirectionInput(di);
    }
    if (ri.scale != 0.0) {
      ic->OnRotationInput(ri);
    }
    ic->OnKey((KeyCode)key, (KeyState)action);
  }
}

void InputSystem::OnMouseClick(int button, int action) {
  // DEBUG_LOG("click, button:{}, action:{}", button, action);
  mouse_state_[button] = action;

  for (const auto& ic : ic_array_) {
    ic->OnMouseClick((MouseButton)button, (MouseState)action);
  }
}

void InputSystem::OnMouseMove(double x, double y) {
  // DEBUG_LOG("move:[{}, {}]", x, y);
  mouse_position_.x = x;
  mouse_position_.y = y;

  for (const auto& ic : ic_array_) {
    ic->OnMouseMove(x, y);
  }
}
}  // namespace lvk