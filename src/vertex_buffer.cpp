#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <cstring>

#include "vertex_buffer.h"
#include "vulkan_utils.h"

namespace VulkanHelpers {

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
    return vk::VertexInputBindingDescription{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex,
    };
}

std::vector<vk::VertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
    std::array<vk::VertexInputAttributeDescription, 3> attributes = {
        vk::VertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = offsetof(Vertex, pos),
        },
        vk::VertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, color),
        },
        vk::VertexInputAttributeDescription{
            .location = 2,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = offsetof(Vertex, texCoord),
        },
    };
    return {attributes.begin(), attributes.end()};
}

VertexBuffer::VertexBuffer(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
    const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const std::vector<Vertex> &vertices
) {
    vertexCount = static_cast<uint32_t>(vertices.size());
    vk::DeviceSize dataSize = sizeof(Vertex) * vertices.size();

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
    std::memcpy(mapped, vertices.data(), static_cast<size_t>(dataSize));
    stagingMemory.unmapMemory();

    // Device-local vertex buffer
    vertexBuffer = std::make_shared<vk::raii::Buffer>(
        device, vk::BufferCreateInfo{
                    .size = dataSize,
                    .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                    .sharingMode = vk::SharingMode::eExclusive,
                }
    );
    auto vertexReqs = vertexBuffer->getMemoryRequirements();
    vertexBufferMemory = std::make_shared<vk::raii::DeviceMemory>(
        device, vk::MemoryAllocateInfo{
                    .allocationSize = vertexReqs.size,
                    .memoryTypeIndex = findMemoryType(physicalDevice, vertexReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal),
                }
    );
    vertexBuffer->bindMemory(**vertexBufferMemory, 0);

    // One-time transfer command
    auto cmdBuffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo{
        .commandPool = *commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    });
    auto &cmd = cmdBuffers[0];
    cmd.begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    cmd.copyBuffer(*stagingBuffer, **vertexBuffer, vk::BufferCopy{.size = dataSize});
    cmd.end();

    vk::CommandBuffer rawCmd = *cmd;
    graphicsQueue.submit(vk::SubmitInfo{.commandBufferCount = 1, .pCommandBuffers = &rawCmd});
    graphicsQueue.waitIdle();
    // stagingBuffer and stagingMemory destroyed here (RAII)
}

} // namespace VulkanHelpers
