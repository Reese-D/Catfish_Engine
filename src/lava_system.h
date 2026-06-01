#ifndef LAVA_SYSTEM_H
#define LAVA_SYSTEM_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <entt/entt.hpp>

#include "draw_call.h"
#include "lava_zone.h"
#include "model.h"

namespace Systems {

// Creates a shared 1×1 unit quad with an orange-red texture.
// Pass the returned model to appendLavaDrawCalls every frame.
std::shared_ptr<VulkanHelpers::Model> createLavaTileModel(
    const vk::raii::Device &device,
    const vk::raii::PhysicalDevice &physicalDevice,
    const vk::raii::CommandPool &commandPool,
    const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
);

// Appends one draw call per lava cell at Z=0.02f so depth testing puts
// unit bodies (which extend above ground) correctly in front of lava.
void appendLavaDrawCalls(
    const LavaZone &lava,
    std::vector<VulkanHelpers::DrawCall> &draws,
    const VulkanHelpers::Model &tileModel
);

// Applies continuous HP drain to every unit (MovementSpeed + Health) in lava.
void applyLavaDamage(const LavaZone &lava, entt::registry &registry, float dt);

} // namespace Systems

#endif // LAVA_SYSTEM_H
