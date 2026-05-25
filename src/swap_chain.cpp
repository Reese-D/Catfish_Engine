#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <algorithm>
#include <array>
#include <limits>

#include "swap_chain.h"

namespace VulkanHelpers {

SwapChain::SwapChain(
    const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::Device &device, const vk::raii::SurfaceKHR &surface, uint32_t graphicsFamily, uint32_t presentFamily,
    const Window &window
) {
    create(physicalDevice, device, surface, graphicsFamily, presentFamily, window);
}

void SwapChain::recreate(
    const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::Device &device, const vk::raii::SurfaceKHR &surface, uint32_t graphicsFamily, uint32_t presentFamily,
    const Window &window
) {
    imageViews.clear();
    swapChain.reset();
    create(physicalDevice, device, surface, graphicsFamily, presentFamily, window);
}

void SwapChain::create(
    const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::Device &device, const vk::raii::SurfaceKHR &surface, uint32_t graphicsFamily, uint32_t presentFamily,
    const Window &window
) {
    auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    auto surfaceFormat = chooseSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(*surface));
    auto presentMode = choosePresentMode(physicalDevice.getSurfacePresentModesKHR(*surface));
    extent = chooseExtent(capabilities, window);
    format = surfaceFormat.format;

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    auto createInfo = vk::SwapchainCreateInfoKHR{
        .surface = *surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = presentMode,
        .clipped = vk::True,
    };

    if (graphicsFamily != presentFamily) {
        std::array<uint32_t, 2> queueFamilyIndices = {graphicsFamily, presentFamily};
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    swapChain = std::make_shared<vk::raii::SwapchainKHR>(device, createInfo);
    images = swapChain->getImages();
    createImageViews(device);
}

void SwapChain::createImageViews(const vk::raii::Device &device) {
    imageViews.clear();
    for (const auto &image : images) {
        auto createInfo = vk::ImageViewCreateInfo{
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = format,
            .subresourceRange = vk::ImageSubresourceRange{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        imageViews.emplace_back(device, createInfo);
    }
}

vk::SurfaceFormatKHR SwapChain::chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &formats) {
    for (const auto &f : formats) {
        if (f.format == vk::Format::eB8G8R8A8Srgb && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return f;
        }
    }
    return formats[0];
}

vk::PresentModeKHR SwapChain::choosePresentMode(const std::vector<vk::PresentModeKHR> &modes) {
    for (const auto &m : modes) {
        if (m == vk::PresentModeKHR::eMailbox) {
            return m;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D SwapChain::chooseExtent(const vk::SurfaceCapabilitiesKHR &capabilities, const Window &window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    auto [fbWidth, fbHeight] = window.getFramebufferSize();
    return vk::Extent2D{
        .width = std::clamp(static_cast<uint32_t>(fbWidth), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        .height = std::clamp(static_cast<uint32_t>(fbHeight), capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };
}

} // namespace VulkanHelpers
