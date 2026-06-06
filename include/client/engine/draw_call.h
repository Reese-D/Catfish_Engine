#ifndef DRAW_CALL_H
#define DRAW_CALL_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

namespace VulkanHelpers {

class Model;

struct DrawCall {
    const Model *model;
    vk::DescriptorSet materialSet; // set 1 — texture, bound per draw
    glm::mat4 transform;
};

struct ProjectileDrawCall {
    const Model *model;
    vk::DescriptorSet materialSet;
    glm::mat4 transform;
    float time{0.0f};
    uint32_t shaderType{0}; // 0=fireball, 1=gravityWell
};

} // namespace VulkanHelpers

#endif // DRAW_CALL_H
