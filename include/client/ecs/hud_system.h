#ifndef HUD_SYSTEM_H
#define HUD_SYSTEM_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <entt/entt.hpp>

#include "draw_call.h"
#include "fog_of_war.h"
#include "model.h"

namespace VulkanHelpers {

struct HudResources {
    std::shared_ptr<Model> barBackground; // grey, full width
    std::shared_ptr<Model> barGreen;      // > 66 % HP
    std::shared_ptr<Model> barYellow;     // 33–66 % HP
    std::shared_ptr<Model> barRed;        // < 33 % HP
};

HudResources createHudResources(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
);

} // namespace VulkanHelpers

namespace Systems {

// Appends background + foreground bar draw calls for every entity with Health.
// fog may be nullptr to disable culling.
void appendHealthBars(entt::registry &registry, std::vector<VulkanHelpers::DrawCall> &draws, const VulkanHelpers::HudResources &hud, const FogOfWar *fog = nullptr);

} // namespace Systems

#endif // HUD_SYSTEM_H
