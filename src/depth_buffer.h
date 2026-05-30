#ifndef DEPTH_BUFFER_H
#define DEPTH_BUFFER_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

class DepthBuffer {
  public:
    DepthBuffer(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, vk::Extent2D extent);
    ~DepthBuffer() = default;

    DepthBuffer(const DepthBuffer &) = delete;
    DepthBuffer &operator=(const DepthBuffer &) = delete;

    void recreate(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, vk::Extent2D extent);

    vk::Image getImage() const { return **image; }
    vk::ImageView getImageView() const { return **imageView; }
    vk::Format getFormat() const { return format; }

  private:
    void create(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, vk::Extent2D extent);
    static vk::Format findDepthFormat(const vk::raii::PhysicalDevice &physicalDevice);

    std::shared_ptr<vk::raii::Image> image;
    std::shared_ptr<vk::raii::DeviceMemory> imageMemory;
    std::shared_ptr<vk::raii::ImageView> imageView;
    vk::Format format{};
};

} // namespace VulkanHelpers

#endif // DEPTH_BUFFER_H
