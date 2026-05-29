#ifndef UNIFORM_BUFFER_H
#define UNIFORM_BUFFER_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class UniformBuffer {
  public:
    UniformBuffer(const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::DescriptorSetLayout &descriptorSetLayout);
    ~UniformBuffer() = default;

    UniformBuffer(const UniformBuffer &) = delete;
    UniformBuffer &operator=(const UniformBuffer &) = delete;

    void update(float elapsedSeconds, vk::Extent2D extent);

    vk::DescriptorSet getDescriptorSet() const { return **descriptorSet; }

  private:
    std::shared_ptr<vk::raii::Buffer> buffer;
    std::shared_ptr<vk::raii::DeviceMemory> bufferMemory;
    void *mappedData{nullptr};

    std::shared_ptr<vk::raii::DescriptorPool> descriptorPool;
    std::shared_ptr<vk::raii::DescriptorSet> descriptorSet;
};

} // namespace VulkanHelpers

#endif // UNIFORM_BUFFER_H
