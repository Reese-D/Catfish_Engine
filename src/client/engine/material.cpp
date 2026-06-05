#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "material.h"

namespace VulkanHelpers {

Material::Material(const vk::raii::Device &m_device, const vk::raii::DescriptorSetLayout &m_textureLayout, const TextureImage &texture) {
    auto poolSize = vk::DescriptorPoolSize{
        .type = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
    };
    m_descriptorPool = std::make_shared<vk::raii::DescriptorPool>(
        m_device, vk::DescriptorPoolCreateInfo{
                    .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                    .maxSets = 1,
                    .poolSizeCount = 1,
                    .pPoolSizes = &poolSize,
                }
    );

    vk::DescriptorSetLayout rawLayout = *m_textureLayout;
    auto sets = m_device.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo{
            .descriptorPool = **m_descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &rawLayout,
        }
    );
    m_descriptorSet = std::make_shared<vk::raii::DescriptorSet>(std::move(sets[0]));

    auto imageInfo = vk::DescriptorImageInfo{
        .sampler = texture.getSampler(),
        .imageView = texture.getImageView(),
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };
    m_device.updateDescriptorSets(
        vk::WriteDescriptorSet{
            .dstSet = **m_descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfo,
        },
        {}
    );
}

} // namespace VulkanHelpers
