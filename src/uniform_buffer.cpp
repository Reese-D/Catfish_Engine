#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <cstring>

#include "uniform_buffer.h"
#include "vulkan_utils.h"

namespace VulkanHelpers {

UniformBuffer::UniformBuffer(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
    const vk::raii::DescriptorSetLayout &descriptorSetLayout,
    vk::ImageView textureImageView, vk::Sampler textureSampler
) {
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

    std::array<vk::DescriptorPoolSize, 2> poolSizes = {
        vk::DescriptorPoolSize{.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1},
        vk::DescriptorPoolSize{.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1},
    };
    descriptorPool = std::make_shared<vk::raii::DescriptorPool>(
        device, vk::DescriptorPoolCreateInfo{
                    .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                    .maxSets = 1,
                    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
                    .pPoolSizes = poolSizes.data(),
                }
    );

    vk::DescriptorSetLayout rawLayout = *descriptorSetLayout;
    auto sets = device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
        .descriptorPool = **descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &rawLayout,
    });
    descriptorSet = std::make_shared<vk::raii::DescriptorSet>(std::move(sets[0]));

    auto bufferInfo = vk::DescriptorBufferInfo{
        .buffer = **buffer,
        .offset = 0,
        .range = sizeof(UniformBufferObject),
    };
    auto imageInfo = vk::DescriptorImageInfo{
        .sampler = textureSampler,
        .imageView = textureImageView,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };
    std::array<vk::WriteDescriptorSet, 2> writes = {
        vk::WriteDescriptorSet{
            .dstSet = **descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo,
        },
        vk::WriteDescriptorSet{
            .dstSet = **descriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfo,
        },
    };
    device.updateDescriptorSets(writes, {});
}

void UniformBuffer::update(const glm::mat4 &view, const glm::mat4 &proj) {
    UniformBufferObject ubo{};
    ubo.view = view;
    ubo.proj = proj;
    std::memcpy(mappedData, &ubo, sizeof(ubo));
}

} // namespace VulkanHelpers
