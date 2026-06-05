#ifndef UNIFORM_BUFFER_H
#define UNIFORM_BUFFER_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <glm/glm.hpp>

#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

struct UniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class UniformBuffer {
  public:
    UniformBuffer(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::DescriptorSetLayout &uboLayout);
    ~UniformBuffer() = default;

    UniformBuffer(const UniformBuffer &) = delete;
    UniformBuffer &operator=(const UniformBuffer &) = delete;

    void update(const glm::mat4 &view, const glm::mat4 &proj);

    vk::DescriptorSet getDescriptorSet() const { return **m_descriptorSet; }

  private:
    std::shared_ptr<vk::raii::Buffer> m_buffer;
    std::shared_ptr<vk::raii::DeviceMemory> m_bufferMemory;
    void *m_mappedData{nullptr};

    std::shared_ptr<vk::raii::DescriptorPool> m_descriptorPool;
    std::shared_ptr<vk::raii::DescriptorSet> m_descriptorSet;
};

} // namespace VulkanHelpers

#endif // UNIFORM_BUFFER_H
