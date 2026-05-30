#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <stdexcept>

#include "depth_buffer.h"
#include "vulkan_utils.h"

namespace VulkanHelpers {

DepthBuffer::DepthBuffer(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, vk::Extent2D extent) {
    create(device, physicalDevice, extent);
}

void DepthBuffer::recreate(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, vk::Extent2D extent) {
    imageView.reset();
    imageMemory.reset();
    image.reset();
    create(device, physicalDevice, extent);
}

void DepthBuffer::create(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, vk::Extent2D extent) {
    format = findDepthFormat(physicalDevice);

    image = std::make_shared<vk::raii::Image>(
        device, vk::ImageCreateInfo{
                    .imageType = vk::ImageType::e2D,
                    .format = format,
                    .extent = vk::Extent3D{.width = extent.width, .height = extent.height, .depth = 1},
                    .mipLevels = 1,
                    .arrayLayers = 1,
                    .samples = vk::SampleCountFlagBits::e1,
                    .tiling = vk::ImageTiling::eOptimal,
                    .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                    .sharingMode = vk::SharingMode::eExclusive,
                    .initialLayout = vk::ImageLayout::eUndefined,
                }
    );

    auto memReqs = image->getMemoryRequirements();
    imageMemory = std::make_shared<vk::raii::DeviceMemory>(
        device, vk::MemoryAllocateInfo{
                    .allocationSize = memReqs.size,
                    .memoryTypeIndex = findMemoryType(physicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal),
                }
    );
    image->bindMemory(**imageMemory, 0);

    imageView = std::make_shared<vk::raii::ImageView>(
        device, vk::ImageViewCreateInfo{
                    .image = **image,
                    .viewType = vk::ImageViewType::e2D,
                    .format = format,
                    .subresourceRange = vk::ImageSubresourceRange{
                        .aspectMask = vk::ImageAspectFlagBits::eDepth,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
                }
    );
}

vk::Format DepthBuffer::findDepthFormat(const vk::raii::PhysicalDevice &physicalDevice) {
    for (auto candidate : {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}) {
        auto props = physicalDevice.getFormatProperties(candidate);
        if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            return candidate;
        }
    }
    throw std::runtime_error("Failed to find supported depth format");
}

} // namespace VulkanHelpers
