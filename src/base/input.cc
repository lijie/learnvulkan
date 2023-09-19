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
    if (static_cast<kInputKeyCode>(key) == kInputKeyCode::W) {
      di.direction = kInputDirection::Forward;
      di.scale = 1.0;
    } else if (static_cast<kInputKeyCode>(key) == kInputKeyCode::S) {
      di.direction = kInputDirection::Forward;
      di.scale = -1.0;
    } else if (static_cast<kInputKeyCode>(key) == kInputKeyCode::D) {
      di.direction = kInputDirection::Right;
      di.scale = -1.0;
    } else if (static_cast<kInputKeyCode>(key) == kInputKeyCode::A) {
      di.direction = kInputDirection::Right;
      di.scale = 1.0;
    } else if (static_cast<kInputKeyCode>(key) == kInputKeyCode::UP) {
      ri.direction = kInputDirection::Forward;
      ri.scale = 1.0;
    } else if (static_cast<kInputKeyCode>(key) == kInputKeyCode::DOWN) {
      ri.direction = kInputDirection::Forward;
      ri.scale = -1.0;
    } else if (static_cast<kInputKeyCode>(key) == kInputKeyCode::LEFT) {
      ri.direction = kInputDirection::Right;
      ri.scale = 1.0;
    } else if (static_cast<kInputKeyCode>(key) == kInputKeyCode::RIGHT) {
      ri.direction = kInputDirection::Right;
      ri.scale = -1.0;
    }
  }

  for (const auto& ic : ic_array_) {
    if (di.scale != 0.0) {
      ic->OnDirectionInput(di);
    } else if (ri.scale != 0.0) {
      ic->OnRotationInput(ri);
    } else {
      ic->OnKey(key, action);
    }
  }
}

}  // namespace lvk