#ifndef TEXTURE_IMAGE_H
#define TEXTURE_IMAGE_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <string>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

class TextureImage {
  public:
    TextureImage(
        const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
        const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
        const std::string &imagePath
    );
    ~TextureImage() = default;

    TextureImage(const TextureImage &) = delete;
    TextureImage &operator=(const TextureImage &) = delete;

    vk::ImageView getImageView() const { return **imageView; }
    vk::Sampler getSampler() const { return **sampler; }

  private:
    std::shared_ptr<vk::raii::Image> image;
    std::shared_ptr<vk::raii::DeviceMemory> imageMemory;
    std::shared_ptr<vk::raii::ImageView> imageView;
    std::shared_ptr<vk::raii::Sampler> sampler;
};

} // namespace VulkanHelpers

#endif // TEXTURE_IMAGE_H
