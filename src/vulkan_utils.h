#ifndef VULKAN_UTILS_H
#define VULKAN_UTILS_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

uint32_t findMemoryType(const vk::raii::PhysicalDevice &physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);

} // namespace VulkanHelpers

#endif // VULKAN_UTILS_H
