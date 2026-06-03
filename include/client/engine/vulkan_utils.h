#ifndef VULKAN_UTILS_H
#define VULKAN_UTILS_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

uint32_t findMemoryType(const vk::raii::PhysicalDevice &physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);

vk::raii::CommandBuffer beginSingleTimeCommands(const vk::raii::Device &device, const vk::raii::CommandPool &commandPool);
void endSingleTimeCommands(vk::raii::CommandBuffer &cmd, const vk::raii::Queue &queue);

} // namespace VulkanHelpers

#endif // VULKAN_UTILS_H
