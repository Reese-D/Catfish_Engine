#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <stdexcept>

#include "vulkan_utils.h"

namespace VulkanHelpers {

uint32_t findMemoryType(const vk::raii::PhysicalDevice &physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    auto memProps = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type");
}

} // namespace VulkanHelpers
