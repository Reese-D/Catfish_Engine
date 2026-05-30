#ifndef SELECTION_RING_H
#define SELECTION_RING_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "model.h"

namespace VulkanHelpers {

// Creates a flat ring mesh in the XY plane. The returned Model is intended to
// be shared and reused for every selected unit — the per-unit position is
// supplied via the draw call's transform matrix.
std::shared_ptr<Model> createSelectionRingModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
    const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout,
    float innerRadius = 0.45f, float outerRadius = 0.65f, int segments = 24
);

} // namespace VulkanHelpers

#endif // SELECTION_RING_H
