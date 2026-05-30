#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <cmath>
#include <numbers>

#include "selection_ring.h"
#include "texture_image.h"
#include "vertex_buffer.h"

namespace VulkanHelpers {

std::shared_ptr<Model> createSelectionRingModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
    const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout,
    float innerRadius, float outerRadius, int segments
) {
    // 1x1 bright yellow pixel — stands out clearly against the green terrain
    std::array<unsigned char, 4> pixels = {255, 220, 30, 255};
    auto texture = std::make_shared<TextureImage>(
        device, physicalDevice, commandPool, graphicsQueue,
        pixels.data(), 1, 1
    );

    // Ring: two concentric circles, outer and inner, N segments each
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(static_cast<std::size_t>(segments * 2));
    indices.reserve(static_cast<std::size_t>(segments * 6));

    for (int i = 0; i < segments; ++i) {
        float angle = static_cast<float>(i) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(segments);
        float cosA  = std::cos(angle);
        float sinA  = std::sin(angle);

        Vertex outer{};
        outer.pos[0]   = outerRadius * cosA;
        outer.pos[1]   = outerRadius * sinA;
        outer.pos[2]   = 0.0f;
        outer.color[0] = outer.color[1] = outer.color[2] = 1.0f;
        outer.texCoord[0] = outer.texCoord[1] = 0.5f;

        Vertex inner{};
        inner.pos[0]   = innerRadius * cosA;
        inner.pos[1]   = innerRadius * sinA;
        inner.pos[2]   = 0.0f;
        inner.color[0] = inner.color[1] = inner.color[2] = 1.0f;
        inner.texCoord[0] = inner.texCoord[1] = 0.5f;

        vertices.push_back(outer);
        vertices.push_back(inner);
    }

    // Quads between adjacent segment pairs
    for (int i = 0; i < segments; ++i) {
        uint32_t o0 = static_cast<uint32_t>(i * 2);         // outer[i]
        uint32_t i0 = static_cast<uint32_t>(i * 2 + 1);     // inner[i]
        uint32_t o1 = static_cast<uint32_t>((i + 1) % segments * 2);     // outer[i+1]
        uint32_t i1 = static_cast<uint32_t>((i + 1) % segments * 2 + 1); // inner[i+1]

        // CCW winding for +Z normal (faces up toward camera)
        indices.insert(indices.end(), {o0, o1, i0, o1, i1, i0});
    }

    return std::make_shared<Model>(
        device, physicalDevice, commandPool, graphicsQueue,
        textureLayout, vertices, indices, std::move(texture)
    );
}

} // namespace VulkanHelpers
