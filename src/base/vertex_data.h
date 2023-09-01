#pragma once

#include <stdint.h>
#include <vector>
#include <vulkan/vulkan.h>

namespace lvk {

struct VertexPosUv {
    float pos[3];
    float uv[2];
};

struct VertexPosNormalUv {
    float pos[3];
    float uv[2];
    float normal[3];
};

inline VertexPosUv MakeVertex(float inpos[3], float inuv[2]) {
    return VertexPosUv{*inpos, *inuv};
}

inline VertexPosNormalUv MakeVertex(float inpos[3], float inuv[2], float innormal[3]) {
    return VertexPosNormalUv{*inpos, *inuv, *innormal};
}

struct VertexInputState {
    std::vector<VkVertexInputAttributeDescription> attribtues;
    uint32_t stride;
};

}