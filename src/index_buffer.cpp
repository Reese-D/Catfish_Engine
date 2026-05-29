#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstring>

#include "index_buffer.h"
#include "vulkan_utils.h"

namespace VulkanHelpers {

IndexBuffer::IndexBuffer(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
    const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const std::vector<uint32_t> &indices
) {
    indexCount = static_cast<uint32_t>(indices.size());
    vk::DeviceSize dataSize = sizeof(uint32_t) * indices.size();

    // Host-visible staging buffer
    auto stagingBuffer = vk::raii::Buffer{
        device, vk::BufferCreateInfo{
                    .size = dataSize,
                    .usage = vk::BufferUsageFlagBits::eTransferSrc,
                    .sharingMode = vk::SharingMode::eExclusive,
                }
    };
    auto stagingReqs = stagingBuffer.getMemoryRequirements();
    auto stagingMemory = vk::raii::DeviceMemory{
        device, vk::MemoryAllocateInfo{
                    .allocationSize = stagingReqs.size,
                    .memoryTypeIndex = findMemoryType(
                        physicalDevice, stagingReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
                    ),
                }
    };
    stagingBuffer.bindMemory(*stagingMemory, 0);

    void *mapped = stagingMemory.mapMemory(0, dataSize);
    std::memcpy(mapped, indices.data(), static_cast<size_t>(dataSize));
    stagingMemory.unmapMemory();

    // Device-local index buffer
    indexBuffer = std::make_shared<vk::raii::Buffer>(
        device, vk::BufferCreateInfo{
                    .size = dataSize,
                    .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                    .sharingMode = vk::SharingMode::eExclusive,
                }
    );
    auto indexReqs = indexBuffer->getMemoryRequirements();
    indexBufferMemory = std::make_shared<vk::raii::DeviceMemory>(
        device, vk::MemoryAllocateInfo{
                    .allocationSize = indexReqs.size,
                    .memoryTypeIndex = findMemoryType(physicalDevice, indexReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal),
                }
    );
    indexBuffer->bindMemory(**indexBufferMemory, 0);

    // One-time transfer command
    auto cmdBuffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
        .commandPool = *commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    });
    auto &cmd = cmdBuffers[0];
    cmd.begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    cmd.copyBuffer(*stagingBuffer, **indexBuffer, vk::BufferCopy{.size = dataSize});
    cmd.end();

    vk::CommandBuffer rawCmd = *cmd;
    graphicsQueue.submit(vk::SubmitInfo{.commandBufferCount = 1, .pCommandBuffers = &rawCmd});
    graphicsQueue.waitIdle();
}

} // namespace VulkanHelpers
