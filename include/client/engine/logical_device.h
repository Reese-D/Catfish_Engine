#ifndef LOGICAL_DEVICE_H
#define LOGICAL_DEVICE_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

class LogicalDevice {
  public:
    LogicalDevice(const vk::raii::PhysicalDevice &physicalDevice, uint32_t graphicsFamily, uint32_t presentFamily);
    ~LogicalDevice() = default;

    LogicalDevice(const LogicalDevice &) = delete;
    LogicalDevice &operator=(const LogicalDevice &) = delete;

    std::shared_ptr<vk::raii::Device> getDevice() const { return m_device; }
    std::shared_ptr<vk::raii::Queue> getGraphicsQueue() const { return m_graphicsQueue; }
    std::shared_ptr<vk::raii::Queue> getPresentQueue() const { return m_presentQueue; }

  private:
    std::shared_ptr<vk::raii::Device> m_device;
    std::shared_ptr<vk::raii::Queue> m_graphicsQueue;
    std::shared_ptr<vk::raii::Queue> m_presentQueue;
};

} // namespace VulkanHelpers

#endif // LOGICAL_DEVICE_H
