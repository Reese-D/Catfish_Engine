#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstring>

#include "uniform_buffer.h"
#include "vulkan_utils.h"

namespace VulkanHelpers {

UniformBuffer::UniformBuffer(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::DescriptorSetLayout &descriptorSetLayout) {
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
                    .memoryTypeIndex = findMemoryType(
                        physicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
                    ),
                }
    );
    buffer->bindMemory(**bufferMemory, 0);
    // Persistently mapped — left mapped for the lifetime of this object
    mappedData = bufferMemory->mapMemory(0, bufferSize);

    // Descriptor pool
    auto poolSize = vk::DescriptorPoolSize{
        .type = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
    };
    descriptorPool = std::make_shared<vk::raii::DescriptorPool>(
        device, vk::DescriptorPoolCreateInfo{
                    .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                    .maxSets = 1,
                    .poolSizeCount = 1,
                    .pPoolSizes = &poolSize,
                }
    );

    // Descriptor set allocation
    vk::DescriptorSetLayout rawLayout = *descriptorSetLayout;
    auto sets = device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
        .descriptorPool = **descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &rawLayout,
    });
    descriptorSet = std::make_shared<vk::raii::DescriptorSet>(std::move(sets[0]));

    // Point the descriptor set at the UBO buffer
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

void UniformBuffer::update(float elapsedSeconds, vk::Extent2D extent) {
    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), elapsedSeconds * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 10.0f);
    ubo.proj[1][1] *= -1; // Flip Y: GLM uses OpenGL convention, Vulkan Y is inverted
    std::memcpy(mappedData, &ubo, sizeof(ubo));
}

} // namespace VulkanHelpers
