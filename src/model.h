#ifndef MODEL_H
#define MODEL_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "index_buffer.h"
#include "material.h"
#include "texture_image.h"
#include "vertex_buffer.h"

namespace VulkanHelpers {

class Model {
  public:
    // Load from a GLB file; creates mesh, texture, and material descriptor set.
    Model(
        const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
        const vk::raii::DescriptorSetLayout &textureLayout, const std::string &path
    );
    // Build from procedural vertex/index data with a pre-created texture.
    Model(
        const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
        const vk::raii::DescriptorSetLayout &textureLayout, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices, std::shared_ptr<TextureImage> texture
    );
    ~Model() = default;

    Model(const Model &) = delete;
    Model &operator=(const Model &) = delete;

    const VertexBuffer &getVertexBuffer() const { return *vertexBuffer; }
    const IndexBuffer &getIndexBuffer() const { return *indexBuffer; }
    const TextureImage &getTextureImage() const { return *textureImage; }
    const Material &getMaterial() const { return *material; }

  private:
    std::shared_ptr<VertexBuffer> vertexBuffer;
    std::shared_ptr<IndexBuffer> indexBuffer;
    std::shared_ptr<TextureImage> textureImage;
    std::shared_ptr<Material> material;
};

} // namespace VulkanHelpers

#endif // MODEL_H
