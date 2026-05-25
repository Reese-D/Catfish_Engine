#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "physical_device.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace VulkanHelpers {

PhysicalDevice::PhysicalDevice(const vk::raii::Instance &instance) {
    auto devices = instance.enumeratePhysicalDevices();
    auto it = std::ranges::find_if(devices, [](const auto &d) { return isDeviceSuitable(d); });
    if (it == devices.end()) {
        throw std::runtime_error("No suitable GPU found");
    }

    physicalDevice = std::make_shared<vk::raii::PhysicalDevice>(std::move(*it));
    graphicsQueueFamilyIndex = findGraphicsQueueFamily(*physicalDevice);
}

bool PhysicalDevice::isDeviceSuitable(const vk::raii::PhysicalDevice &device) {
    if (device.getProperties().apiVersion < VK_API_VERSION_1_3) {
        return false;
    }

    auto queueFamilies = device.getQueueFamilyProperties();
    if (!std::ranges::any_of(queueFamilies, [](const auto &qf) { return static_cast<bool>(qf.queueFlags & vk::QueueFlagBits::eGraphics); })) {
        return false;
    }

    auto extensions = device.enumerateDeviceExtensionProperties();
    if (!std::ranges::any_of(extensions, [](const auto &ext) { return std::string_view(ext.extensionName.data()) == vk::KHRSwapchainExtensionName; })) {
        return false;
    }

    auto features = device.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceDynamicRenderingFeatures>();
    if (!features.get<vk::PhysicalDeviceDynamicRenderingFeatures>().dynamicRendering) {
        return false;
    }

    return true;
}

uint32_t PhysicalDevice::findGraphicsQueueFamily(const vk::raii::PhysicalDevice &device) {
    auto queueFamilies = device.getQueueFamilyProperties();
    auto it = std::ranges::find_if(queueFamilies, [](const auto &qf) { return static_cast<bool>(qf.queueFlags & vk::QueueFlagBits::eGraphics); });
    return static_cast<uint32_t>(std::distance(queueFamilies.begin(), it));
}

} // namespace VulkanHelpers
