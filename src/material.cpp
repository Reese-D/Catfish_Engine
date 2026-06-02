#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "material.h"

namespace VulkanHelpers {

Material::Material(const vk::raii::Device &device, const vk::raii::DescriptorSetLayout &textureLayout, const TextureImage &texture) {
    auto poolSize = vk::DescriptorPoolSize{
        .type = vk::DescriptorType::eCombinedImageSampler,
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

    vk::DescriptorSetLayout rawLayout = *textureLayout;
    auto sets = device.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo{
            .descriptorPool = **descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &rawLayout,
        }
    );
    descriptorSet = std::make_shared<vk::raii::DescriptorSet>(std::move(sets[0]));

    auto imageInfo = vk::DescriptorImageInfo{
        .sampler = texture.getSampler(),
        .imageView = texture.getImageView(),
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };
    device.updateDescriptorSets(
        vk::WriteDescriptorSet{
            .dstSet = **descriptorSet,
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
