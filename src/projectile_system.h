#ifndef PROJECTILE_SYSTEM_H
#define PROJECTILE_SYSTEM_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "components.h"
#include "model.h"
#include "window.h"

namespace Systems {

// Creates a flat disc mesh to be shared across all projectile entities.
std::shared_ptr<VulkanHelpers::Model> createProjectileModel(
    const vk::raii::Device &device,
    const vk::raii::PhysicalDevice &physicalDevice,
    const vk::raii::CommandPool &commandPool,
    const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
);

// Spawns one projectile entity.
void spawnProjectile(
    entt::registry &registry,
    std::shared_ptr<VulkanHelpers::Model> model,
    glm::vec3 origin,
    glm::vec3 velocity,
    Components::FactionId ownerFaction,
    float knockbackForce,
    float hitRadius = 0.4f,
    float lifetime  = 6.0f
);

// Ticks Ability cooldown timers.
void tickAbilities(entt::registry &registry, float dt);

// Moves projectiles, checks collisions, applies knockback, despawns on hit or expiry.
void updateProjectiles(entt::registry &registry, float dt);

// Reads Q key + mouse position; fires ability for each selected player unit.
void processAbilityInput(
    entt::registry &registry,
    const VulkanHelpers::Window &window,
    vk::Extent2D extent,
    std::shared_ptr<VulkanHelpers::Model> projectileModel
);

} // namespace Systems

#endif // PROJECTILE_SYSTEM_H
