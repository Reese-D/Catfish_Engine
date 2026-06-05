#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "physical_device.h"
#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace VulkanHelpers {

PhysicalDevice::PhysicalDevice(const vk::raii::Instance &instance, const vk::raii::SurfaceKHR &surface) {
    auto devices = instance.enumeratePhysicalDevices();
    auto it = std::ranges::find_if(devices, [&surface](const auto &d) { return isDeviceSuitable(d, surface); });
    if (it == devices.end()) {
        throw std::runtime_error("No suitable GPU found");
    }
    m_physicalDevice = std::make_shared<vk::raii::PhysicalDevice>(std::move(*it));
    m_graphicsQueueFamilyIndex = findGraphicsQueueFamily(*m_physicalDevice);
    m_presentQueueFamilyIndex = findPresentQueueFamily(*m_physicalDevice, surface);
}

bool PhysicalDevice::isDeviceSuitable(const vk::raii::PhysicalDevice &device, const vk::raii::SurfaceKHR &surface) {
    if (device.getProperties().apiVersion < VK_API_VERSION_1_3) {
        return false;
    }

    auto queueFamilies = device.getQueueFamilyProperties();
    if (!std::ranges::any_of(queueFamilies, [](const auto &qf) { return static_cast<bool>(qf.queueFlags & vk::QueueFlagBits::eGraphics); })) {
        return false;
    }

    bool hasPresentFamily = false;
    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        if (device.getSurfaceSupportKHR(i, *surface)) {
            hasPresentFamily = true;
            break;
        }
    }
    if (!hasPresentFamily) {
        return false;
    }

    auto extensions = device.enumerateDeviceExtensionProperties();
    if (!std::ranges::any_of(extensions, [](const auto &ext) { return std::string_view(ext.extensionName.data()) == vk::KHRSwapchainExtensionName; })) {
        return false;
    }

    auto formats = device.getSurfaceFormatsKHR(*surface);
    auto presentModes = device.getSurfacePresentModesKHR(*surface);
    if (formats.empty() || presentModes.empty()) {
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

uint32_t PhysicalDevice::findPresentQueueFamily(const vk::raii::PhysicalDevice &device, const vk::raii::SurfaceKHR &surface) {
    auto queueFamilies = device.getQueueFamilyProperties();
    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilies.size()); ++i) {
        if (device.getSurfaceSupportKHR(i, *surface)) {
            return i;
        }
    }
    throw std::runtime_error("No present queue family found");
}

} // namespace VulkanHelpers
