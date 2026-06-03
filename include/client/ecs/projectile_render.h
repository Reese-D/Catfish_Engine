#ifndef PROJECTILE_RENDER_H
#define PROJECTILE_RENDER_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vulkan/vulkan_raii.hpp>

#include "model.h"

namespace Systems {

// Creates the flat disc mesh used to render projectiles.
std::shared_ptr<VulkanHelpers::Model> createProjectileModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
    const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
);

} // namespace Systems

#endif // PROJECTILE_RENDER_H
