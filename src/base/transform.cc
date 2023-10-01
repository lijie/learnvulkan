#include "transform.h"

namespace lvk {
Transform Transform::Identity = {
    .translation{0},
    .rotation{0},
    .scale{1},
};
}