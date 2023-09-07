#include "scene.h"

namespace lvk {
Node* Scene::AddNode(int meshIdx, int matIdx, const Transform& transform)
{
    auto n = new Node{
        .transform = transform,
        .material = matIdx,
        .mesh = meshIdx,
    };
    nodeList_.push_back(n);
    return n;
}
}