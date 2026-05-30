#ifndef TEXTURE_IMAGE_H
#define TEXTURE_IMAGE_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <cstddef>
#include <memory>
#include <span>
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
    TextureImage(
        const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
        const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
        std::span<const std::byte> encodedBytes
    );
    // Construct from raw RGBA pixels (no image decoding step)
    TextureImage(
        const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
        const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
        const unsigned char *pixels, int width, int height
    );
    ~TextureImage() = default;

    TextureImage(const TextureImage &) = delete;
    TextureImage &operator=(const TextureImage &) = delete;

    vk::ImageView getImageView() const { return **imageView; }
    vk::Sampler getSampler() const { return **sampler; }

  private:
    void upload(
        const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
        const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
        const unsigned char *pixels, int texWidth, int texHeight
    );

    std::shared_ptr<vk::raii::Image> image;
    std::shared_ptr<vk::raii::DeviceMemory> imageMemory;
    std::shared_ptr<vk::raii::ImageView> imageView;
    std::shared_ptr<vk::raii::Sampler> sampler;
};

} // namespace VulkanHelpers

#endif // TEXTURE_IMAGE_H
