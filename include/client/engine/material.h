#ifndef MATERIAL_H
#define MATERIAL_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "texture_image.h"

namespace VulkanHelpers {

// Owns the per-draw descriptor set for set 1 (texture sampler).
class Material {
  public:
    Material(const vk::raii::Device &device, const vk::raii::DescriptorSetLayout &textureLayout, const TextureImage &texture);
    ~Material() = default;

    Material(const Material &) = delete;
    Material &operator=(const Material &) = delete;

    vk::DescriptorSet getDescriptorSet() const { return **descriptorSet; }

  private:
    std::shared_ptr<vk::raii::DescriptorPool> descriptorPool;
    std::shared_ptr<vk::raii::DescriptorSet> descriptorSet;
};

} // namespace VulkanHelpers

#endif // MATERIAL_H
