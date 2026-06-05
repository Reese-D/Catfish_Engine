#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstring>

#include "uniform_buffer.h"
#include "vulkan_utils.h"

namespace VulkanHelpers {

UniformBuffer::UniformBuffer(const vk::raii::Device &m_device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::DescriptorSetLayout &m_uboLayout) {
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    m_buffer = std::make_shared<vk::raii::Buffer>(
        m_device, vk::BufferCreateInfo{
                    .size = bufferSize,
                    .usage = vk::BufferUsageFlagBits::eUniformBuffer,
                    .sharingMode = vk::SharingMode::eExclusive,
                }
    );
    auto memReqs = m_buffer->getMemoryRequirements();
    m_bufferMemory = std::make_shared<vk::raii::DeviceMemory>(
        m_device, vk::MemoryAllocateInfo{
                    .allocationSize = memReqs.size,
                    .memoryTypeIndex = findMemoryType(physicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent),
                }
    );
    m_buffer->bindMemory(**m_bufferMemory, 0);
    m_mappedData = m_bufferMemory->mapMemory(0, bufferSize);

    auto poolSize = vk::DescriptorPoolSize{.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1};
    m_descriptorPool = std::make_shared<vk::raii::DescriptorPool>(
        m_device, vk::DescriptorPoolCreateInfo{
                    .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                    .maxSets = 1,
                    .poolSizeCount = 1,
                    .pPoolSizes = &poolSize,
                }
    );

    vk::DescriptorSetLayout rawLayout = *m_uboLayout;
    auto sets = m_device.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo{
            .descriptorPool = **m_descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &rawLayout,
        }
    );
    m_descriptorSet = std::make_shared<vk::raii::DescriptorSet>(std::move(sets[0]));

    auto bufferInfo = vk::DescriptorBufferInfo{
        .buffer = **m_buffer,
        .offset = 0,
        .range = sizeof(UniformBufferObject),
    };
    m_device.updateDescriptorSets(
        vk::WriteDescriptorSet{
            .dstSet = **m_descriptorSet,
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
    std::memcpy(m_mappedData, &ubo, sizeof(ubo));
}

} // namespace VulkanHelpers
