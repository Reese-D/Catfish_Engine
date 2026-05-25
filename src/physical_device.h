#ifndef PHYSICAL_DEVICE_H
#define PHYSICAL_DEVICE_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

class PhysicalDevice {
  public:
    PhysicalDevice(const vk::raii::Instance &instance);
    ~PhysicalDevice() = default;

    PhysicalDevice(const PhysicalDevice &) = delete;
    PhysicalDevice &operator=(const PhysicalDevice &) = delete;

    std::shared_ptr<vk::raii::PhysicalDevice> getPhysicalDevice() const { return physicalDevice; }
    uint32_t getGraphicsQueueFamilyIndex() const { return graphicsQueueFamilyIndex; }

  private:
    static bool isDeviceSuitable(const vk::raii::PhysicalDevice &device);
    static uint32_t findGraphicsQueueFamily(const vk::raii::PhysicalDevice &device);

    std::shared_ptr<vk::raii::PhysicalDevice> physicalDevice;
    uint32_t graphicsQueueFamilyIndex{0};
};

} // namespace VulkanHelpers

#endif // PHYSICAL_DEVICE_H
