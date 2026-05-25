#ifndef LOGICAL_DEVICE_H
#define LOGICAL_DEVICE_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

class LogicalDevice {
  public:
    LogicalDevice(const vk::raii::PhysicalDevice &physicalDevice, uint32_t graphicsQueueFamilyIndex);
    ~LogicalDevice() = default;

    LogicalDevice(const LogicalDevice &) = delete;
    LogicalDevice &operator=(const LogicalDevice &) = delete;

    std::shared_ptr<vk::raii::Device> getDevice() const { return device; }
    std::shared_ptr<vk::raii::Queue> getGraphicsQueue() const { return graphicsQueue; }

  private:
    std::shared_ptr<vk::raii::Device> device;
    std::shared_ptr<vk::raii::Queue> graphicsQueue;
};

} // namespace VulkanHelpers

#endif // LOGICAL_DEVICE_H
