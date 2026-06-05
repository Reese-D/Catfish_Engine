#ifndef INDEX_BUFFER_H
#define INDEX_BUFFER_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

class IndexBuffer {
  public:
    IndexBuffer(
        const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
        const std::vector<uint32_t> &indices
    );
    ~IndexBuffer() = default;

    IndexBuffer(const IndexBuffer &) = delete;
    IndexBuffer &operator=(const IndexBuffer &) = delete;

    std::shared_ptr<vk::raii::Buffer> getBuffer() const { return m_indexBuffer; }
    uint32_t getIndexCount() const { return m_indexCount; }

  private:
    std::shared_ptr<vk::raii::Buffer> m_indexBuffer;
    std::shared_ptr<vk::raii::DeviceMemory> m_indexBufferMemory;
    uint32_t m_indexCount{0};
};

} // namespace VulkanHelpers

#endif // INDEX_BUFFER_H
