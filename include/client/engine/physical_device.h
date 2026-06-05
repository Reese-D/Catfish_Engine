#ifndef PHYSICAL_DEVICE_H
#define PHYSICAL_DEVICE_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

class PhysicalDevice {
  public:
    PhysicalDevice(const vk::raii::Instance &instance, const vk::raii::SurfaceKHR &surface);
    ~PhysicalDevice() = default;

    PhysicalDevice(const PhysicalDevice &) = delete;
    PhysicalDevice &operator=(const PhysicalDevice &) = delete;

    std::shared_ptr<vk::raii::PhysicalDevice> getPhysicalDevice() const { return m_physicalDevice; }
    uint32_t getGraphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }
    uint32_t getPresentQueueFamilyIndex() const { return m_presentQueueFamilyIndex; }

  private:
    static bool isDeviceSuitable(const vk::raii::PhysicalDevice &device, const vk::raii::SurfaceKHR &surface);
    static uint32_t findGraphicsQueueFamily(const vk::raii::PhysicalDevice &device);
    static uint32_t findPresentQueueFamily(const vk::raii::PhysicalDevice &device, const vk::raii::SurfaceKHR &surface);

    std::shared_ptr<vk::raii::PhysicalDevice> m_physicalDevice;
    uint32_t m_graphicsQueueFamilyIndex{0};
    uint32_t m_presentQueueFamilyIndex{0};
};

} // namespace VulkanHelpers

#endif // PHYSICAL_DEVICE_H
