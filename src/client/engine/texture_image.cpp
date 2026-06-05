#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cstring>
#include <stdexcept>

#include "texture_image.h"
#include "vulkan_utils.h"

namespace VulkanHelpers {

TextureImage::TextureImage(
    const vk::raii::Device &mDevice, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const std::string &imagePath
) {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load(imagePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("Failed to load texture: " + imagePath);
    }
    upload(mDevice, physicalDevice, commandPool, graphicsQueue, pixels, texWidth, texHeight);
    stbi_image_free(pixels);
}

TextureImage::TextureImage(
    const vk::raii::Device &mDevice, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    std::span<const std::byte> encodedBytes
) {
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels =
        stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(encodedBytes.data()), static_cast<int>(encodedBytes.size()), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("Failed to decode embedded texture");
    }
    upload(mDevice, physicalDevice, commandPool, graphicsQueue, pixels, texWidth, texHeight);
    stbi_image_free(pixels);
}

TextureImage::TextureImage(
    const vk::raii::Device &mDevice, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const unsigned char *pixels, int width, int height
) {
    upload(mDevice, physicalDevice, commandPool, graphicsQueue, pixels, width, height);
}

void TextureImage::upload(
    const vk::raii::Device &mDevice, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const unsigned char *pixels, int texWidth, int texHeight
) {
    vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(texWidth) * texHeight * 4;

    auto stagingBuffer = vk::raii::Buffer{
        mDevice, vk::BufferCreateInfo{
                     .size = imageSize,
                     .usage = vk::BufferUsageFlagBits::eTransferSrc,
                     .sharingMode = vk::SharingMode::eExclusive,
                 }
    };
    auto stagingReqs = stagingBuffer.getMemoryRequirements();
    auto stagingMemory = vk::raii::DeviceMemory{
        mDevice, vk::MemoryAllocateInfo{
                     .allocationSize = stagingReqs.size,
                     .memoryTypeIndex = findMemoryType(physicalDevice, stagingReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent),
                 }
    };
    stagingBuffer.bindMemory(*stagingMemory, 0);

    void *mapped = stagingMemory.mapMemory(0, imageSize);
    std::memcpy(mapped, pixels, static_cast<size_t>(imageSize));
    stagingMemory.unmapMemory();

    m_image = std::make_shared<vk::raii::Image>(
        mDevice, vk::ImageCreateInfo{
                     .imageType = vk::ImageType::e2D,
                     .format = vk::Format::eR8G8B8A8Srgb,
                     .extent =
                         vk::Extent3D{
                             .width = static_cast<uint32_t>(texWidth),
                             .height = static_cast<uint32_t>(texHeight),
                             .depth = 1,
                         },
                     .mipLevels = 1,
                     .arrayLayers = 1,
                     .samples = vk::SampleCountFlagBits::e1,
                     .tiling = vk::ImageTiling::eOptimal,
                     .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                     .sharingMode = vk::SharingMode::eExclusive,
                     .initialLayout = vk::ImageLayout::eUndefined,
                 }
    );
    auto imgReqs = m_image->getMemoryRequirements();
    m_imageMemory = std::make_shared<vk::raii::DeviceMemory>(
        mDevice, vk::MemoryAllocateInfo{
                     .allocationSize = imgReqs.size,
                     .memoryTypeIndex = findMemoryType(physicalDevice, imgReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal),
                 }
    );
    m_image->bindMemory(**m_imageMemory, 0);

    auto colorRange = vk::ImageSubresourceRange{
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    {
        auto cmd = beginSingleTimeCommands(mDevice, commandPool);
        auto barrier = vk::ImageMemoryBarrier2{
            .srcStageMask = vk::PipelineStageFlagBits2::eNone,
            .srcAccessMask = vk::AccessFlagBits2::eNone,
            .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .oldLayout = vk::ImageLayout::eUndefined,
            .newLayout = vk::ImageLayout::eTransferDstOptimal,
            .image = **m_image,
            .subresourceRange = colorRange,
        };
        cmd.pipelineBarrier2(vk::DependencyInfo{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier});
        endSingleTimeCommands(cmd, graphicsQueue);
    }

    {
        auto cmd = beginSingleTimeCommands(mDevice, commandPool);
        auto region = vk::BufferImageCopy{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource =
                vk::ImageSubresourceLayers{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            .imageOffset = vk::Offset3D{0, 0, 0},
            .imageExtent = vk::Extent3D{static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1},
        };
        cmd.copyBufferToImage(*stagingBuffer, **m_image, vk::ImageLayout::eTransferDstOptimal, region);
        endSingleTimeCommands(cmd, graphicsQueue);
    }

    {
        auto cmd = beginSingleTimeCommands(mDevice, commandPool);
        auto barrier = vk::ImageMemoryBarrier2{
            .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
            .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .image = **m_image,
            .subresourceRange = colorRange,
        };
        cmd.pipelineBarrier2(vk::DependencyInfo{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier});
        endSingleTimeCommands(cmd, graphicsQueue);
    }

    m_imageView = std::make_shared<vk::raii::ImageView>(
        mDevice, vk::ImageViewCreateInfo{
                     .image = **m_image,
                     .viewType = vk::ImageViewType::e2D,
                     .format = vk::Format::eR8G8B8A8Srgb,
                     .subresourceRange = colorRange,
                 }
    );

    auto limits = physicalDevice.getProperties().limits;
    m_sampler = std::make_shared<vk::raii::Sampler>(
        mDevice, vk::SamplerCreateInfo{
                     .magFilter = vk::Filter::eLinear,
                     .minFilter = vk::Filter::eLinear,
                     .mipmapMode = vk::SamplerMipmapMode::eLinear,
                     .addressModeU = vk::SamplerAddressMode::eRepeat,
                     .addressModeV = vk::SamplerAddressMode::eRepeat,
                     .addressModeW = vk::SamplerAddressMode::eRepeat,
                     .mipLodBias = 0.0f,
                     .anisotropyEnable = vk::True,
                     .maxAnisotropy = limits.maxSamplerAnisotropy,
                     .compareEnable = vk::False,
                     .compareOp = vk::CompareOp::eAlways,
                     .minLod = 0.0f,
                     .maxLod = 0.0f,
                     .borderColor = vk::BorderColor::eIntOpaqueBlack,
                     .unnormalizedCoordinates = vk::False,
                 }
    );
}

} // namespace VulkanHelpers
