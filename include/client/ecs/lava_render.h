#ifndef LAVA_RENDER_H
#define LAVA_RENDER_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "draw_call.h"
#include "lava_zone.h"
#include "model.h"

namespace Systems {

// Creates a shared 1×1 unit quad with an orange-red texture.
std::shared_ptr<VulkanHelpers::Model> createLavaTileModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
    const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
);

// Appends one draw call per lava cell.
void appendLavaDrawCalls(
    const LavaZone &lava, std::vector<VulkanHelpers::DrawCall> &draws,
    const VulkanHelpers::Model &tileModel
);

} // namespace Systems

#endif // LAVA_RENDER_H
