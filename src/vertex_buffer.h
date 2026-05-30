#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

struct Vertex {
    float pos[3];
    float color[3];
    float texCoord[2];

    static vk::VertexInputBindingDescription getBindingDescription();
    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions();
};

class VertexBuffer {
  public:
    VertexBuffer(
        const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
        const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
        const std::vector<Vertex> &vertices
    );
    ~VertexBuffer() = default;

    VertexBuffer(const VertexBuffer &) = delete;
    VertexBuffer &operator=(const VertexBuffer &) = delete;

    std::shared_ptr<vk::raii::Buffer> getBuffer() const { return vertexBuffer; }
    uint32_t getVertexCount() const { return vertexCount; }

  private:
    std::shared_ptr<vk::raii::Buffer> vertexBuffer;
    std::shared_ptr<vk::raii::DeviceMemory> vertexBufferMemory;
    uint32_t vertexCount{0};
};

} // namespace VulkanHelpers

#endif // VERTEX_BUFFER_H
