#ifndef MODEL_H
#define MODEL_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <string>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "index_buffer.h"
#include "texture_image.h"
#include "vertex_buffer.h"

namespace VulkanHelpers {

class Model {
  public:
    Model(
        const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
        const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
        const std::string &path
    );
    ~Model() = default;

    Model(const Model &) = delete;
    Model &operator=(const Model &) = delete;

    const VertexBuffer &getVertexBuffer() const { return *vertexBuffer; }
    const IndexBuffer &getIndexBuffer() const { return *indexBuffer; }
    const TextureImage &getTextureImage() const { return *textureImage; }

  private:
    std::shared_ptr<VertexBuffer> vertexBuffer;
    std::shared_ptr<IndexBuffer> indexBuffer;
    std::shared_ptr<TextureImage> textureImage;
};

} // namespace VulkanHelpers

#endif // MODEL_H
