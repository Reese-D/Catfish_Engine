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

    vk::Image getImage() const { return **m_image; }
    vk::ImageView getImageView() const { return **m_imageView; }
    vk::Format getFormat() const { return m_format; }

  private:
    void create(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, vk::Extent2D extent);
    static vk::Format findDepthFormat(const vk::raii::PhysicalDevice &physicalDevice);

    std::shared_ptr<vk::raii::Image> m_image;
    std::shared_ptr<vk::raii::DeviceMemory> m_imageMemory;
    std::shared_ptr<vk::raii::ImageView> m_imageView;
    vk::Format m_format{};
};

} // namespace VulkanHelpers

#endif // DEPTH_BUFFER_H
