#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <set>
#include <vector>

#include "logical_device.h"

namespace VulkanHelpers {

LogicalDevice::LogicalDevice(const vk::raii::PhysicalDevice &physicalDevice, uint32_t graphicsFamily, uint32_t presentFamily) {
    float queuePriority = 0.5f;
    std::set<uint32_t> uniqueFamilies = {graphicsFamily, presentFamily};
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t family : uniqueFamilies) {
        queueCreateInfos.push_back(
            vk::DeviceQueueCreateInfo{
                .queueFamilyIndex = family,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority,
            }
        );
    }

    auto vulkan13Features = vk::PhysicalDeviceVulkan13Features{
        .synchronization2 = vk::True,
        .dynamicRendering = vk::True,
    };
    auto features2 = vk::PhysicalDeviceFeatures2{
        .pNext = &vulkan13Features,
        .features = vk::PhysicalDeviceFeatures{
            .samplerAnisotropy = vk::True,
        },
    };

    std::array<const char *, 1> deviceExtensions = {vk::KHRSwapchainExtensionName};

    auto deviceCreateInfo = vk::DeviceCreateInfo{
        .pNext = &features2,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
    };

    device = std::make_shared<vk::raii::Device>(physicalDevice, deviceCreateInfo);
    graphicsQueue = std::make_shared<vk::raii::Queue>(*device, graphicsFamily, 0);
    presentQueue = std::make_shared<vk::raii::Queue>(*device, presentFamily, 0);
}

} // namespace VulkanHelpers
