#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstring>

#include "uniform_buffer.h"
#include "vulkan_utils.h"

namespace VulkanHelpers {

UniformBuffer::UniformBuffer(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::DescriptorSetLayout &uboLayout) {
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    buffer = std::make_shared<vk::raii::Buffer>(
        device, vk::BufferCreateInfo{
                    .size = bufferSize,
                    .usage = vk::BufferUsageFlagBits::eUniformBuffer,
                    .sharingMode = vk::SharingMode::eExclusive,
                }
    );
    auto memReqs = buffer->getMemoryRequirements();
    bufferMemory = std::make_shared<vk::raii::DeviceMemory>(
        device, vk::MemoryAllocateInfo{
                    .allocationSize = memReqs.size,
                    .memoryTypeIndex = findMemoryType(physicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent),
                }
    );
    buffer->bindMemory(**bufferMemory, 0);
    mappedData = bufferMemory->mapMemory(0, bufferSize);

    auto poolSize = vk::DescriptorPoolSize{.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1};
    descriptorPool = std::make_shared<vk::raii::DescriptorPool>(
        device, vk::DescriptorPoolCreateInfo{
                    .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                    .maxSets = 1,
                    .poolSizeCount = 1,
                    .pPoolSizes = &poolSize,
                }
    );

    vk::DescriptorSetLayout rawLayout = *uboLayout;
    auto sets = device.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo{
            .descriptorPool = **descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &rawLayout,
        }
    );
    descriptorSet = std::make_shared<vk::raii::DescriptorSet>(std::move(sets[0]));

    auto bufferInfo = vk::DescriptorBufferInfo{
        .buffer = **buffer,
        .offset = 0,
        .range = sizeof(UniformBufferObject),
    };
    device.updateDescriptorSets(
        vk::WriteDescriptorSet{
            .dstSet = **descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo,
        },
        {}
    );
}

void UniformBuffer::update(const glm::mat4 &view, const glm::mat4 &proj) {
    UniformBufferObject ubo{};
    ubo.view = view;
    ubo.proj = proj;
    std::memcpy(mappedData, &ubo, sizeof(ubo));
}

} // namespace VulkanHelpers
