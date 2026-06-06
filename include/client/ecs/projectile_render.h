#ifndef PROJECTILE_RENDER_H
#define PROJECTILE_RENDER_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include <entt/entt.hpp>

#include "draw_call.h"
#include "model.h"

namespace Systems {

// Flat disc mesh for the fireball projectile (radius 0.25).
std::shared_ptr<VulkanHelpers::Model> createProjectileModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
);

// Larger flat disc mesh for the gravity well (radius 0.5).
std::shared_ptr<VulkanHelpers::Model> createGravityWellModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
);

// Collects ProjectileDrawCalls for all Projectile entities in the registry.
void appendProjectileDrawCalls(
    entt::registry &registry, std::vector<VulkanHelpers::ProjectileDrawCall> &draws, const VulkanHelpers::Model &fireballModel, const VulkanHelpers::Model &gravityWellModel,
    float time
);

} // namespace Systems

#endif // PROJECTILE_RENDER_H
