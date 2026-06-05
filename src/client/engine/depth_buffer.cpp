#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <stdexcept>

#include "depth_buffer.h"
#include "vulkan_utils.h"

namespace VulkanHelpers {

DepthBuffer::DepthBuffer(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, vk::Extent2D mExtent) { create(device, physicalDevice, mExtent); }

void DepthBuffer::recreate(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, vk::Extent2D mExtent) {
    m_imageView.reset();
    m_imageMemory.reset();
    m_image.reset();
    create(device, physicalDevice, mExtent);
}

void DepthBuffer::create(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, vk::Extent2D mExtent) {
    m_format = findDepthFormat(physicalDevice);

    m_image = std::make_shared<vk::raii::Image>(
        device, vk::ImageCreateInfo{
                    .imageType = vk::ImageType::e2D,
                    .format = m_format,
                    .extent = vk::Extent3D{.width = mExtent.width, .height = mExtent.height, .depth = 1},
                    .mipLevels = 1,
                    .arrayLayers = 1,
                    .samples = vk::SampleCountFlagBits::e1,
                    .tiling = vk::ImageTiling::eOptimal,
                    .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                    .sharingMode = vk::SharingMode::eExclusive,
                    .initialLayout = vk::ImageLayout::eUndefined,
                }
    );

    auto memReqs = m_image->getMemoryRequirements();
    m_imageMemory = std::make_shared<vk::raii::DeviceMemory>(
        device, vk::MemoryAllocateInfo{
                    .allocationSize = memReqs.size,
                    .memoryTypeIndex = findMemoryType(physicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal),
                }
    );
    m_image->bindMemory(**m_imageMemory, 0);

    m_imageView = std::make_shared<vk::raii::ImageView>(
        device, vk::ImageViewCreateInfo{
                    .image = **m_image,
                    .viewType = vk::ImageViewType::e2D,
                    .format = m_format,
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
    throw std::runtime_error("Failed to find supported depth m_format");
}

} // namespace VulkanHelpers
