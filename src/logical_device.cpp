#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>

#include "logical_device.h"

namespace VulkanHelpers {

LogicalDevice::LogicalDevice(const vk::raii::PhysicalDevice &physicalDevice, uint32_t graphicsQueueFamilyIndex) {
    float queuePriority = 0.5f;
    auto queueCreateInfo = vk::DeviceQueueCreateInfo{
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    auto vulkan13Features = vk::PhysicalDeviceVulkan13Features{
        .dynamicRendering = vk::True,
    };
    auto features2 = vk::PhysicalDeviceFeatures2{
        .pNext = &vulkan13Features,
    };

    std::array<const char *, 1> deviceExtensions = {vk::KHRSwapchainExtensionName};

    auto deviceCreateInfo = vk::DeviceCreateInfo{
        .pNext = &features2,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
    };

    device = std::make_shared<vk::raii::Device>(physicalDevice, deviceCreateInfo);
    graphicsQueue = std::make_shared<vk::raii::Queue>(*device, graphicsQueueFamilyIndex, 0);
}

} // namespace VulkanHelpers
