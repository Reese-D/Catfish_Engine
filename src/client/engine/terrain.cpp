#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>
#include <cstdint>

#include "terrain.h"
#include "texture_image.h"
#include "vertex_buffer.h"

namespace VulkanHelpers {

Terrain::Terrain(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout, float extent, int cells
) {
    // 4x4 checkerboard grass texture (two shades of green, full RGBA)
    constexpr int kTexSize = 4;
    std::array<unsigned char, kTexSize * kTexSize * 4> pixels{};
    for (int y = 0; y < kTexSize; ++y) {
        for (int x = 0; x < kTexSize; ++x) {
            bool light = (x + y) % 2 == 0;
            int i = (y * kTexSize + x) * 4;
            pixels[i] = light ? 80 : 50;       // R
            pixels[i + 1] = light ? 150 : 100; // G
            pixels[i + 2] = light ? 60 : 40;   // B
            pixels[i + 3] = 255;               // A
        }
    }
    auto texture = std::make_shared<TextureImage>(device, physicalDevice, commandPool, graphicsQueue, pixels.data(), kTexSize, kTexSize);

    // Flat grid mesh: (cells+1)^2 vertices, cells^2 quads
    const int verts = cells + 1;
    const float step = (2.0f * extent) / static_cast<float>(cells);
    const float uvScale = static_cast<float>(cells) / 4.0f; // tile the 4px texture

    std::vector<Vertex> vertices;
    vertices.reserve(static_cast<std::size_t>(verts * verts));
    for (int y = 0; y < verts; ++y) {
        for (int x = 0; x < verts; ++x) {
            Vertex v{};
            v.pos[0] = -extent + x * step;
            v.pos[1] = -extent + y * step;
            v.pos[2] = 0.0f;
            v.color[0] = v.color[1] = v.color[2] = 1.0f;
            v.texCoord[0] = static_cast<float>(x) / static_cast<float>(cells) * uvScale;
            v.texCoord[1] = static_cast<float>(y) / static_cast<float>(cells) * uvScale;
            vertices.push_back(v);
        }
    }

    std::vector<uint32_t> indices;
    indices.reserve(static_cast<std::size_t>(cells * cells * 6));
    for (int y = 0; y < cells; ++y) {
        for (int x = 0; x < cells; ++x) {
            uint32_t tl = static_cast<uint32_t>(y * verts + x);
            uint32_t tr = tl + 1;
            uint32_t bl = tl + static_cast<uint32_t>(verts);
            uint32_t br = bl + 1;
            indices.insert(indices.end(), {tl, tr, bl, tr, br, bl});
        }
    }

    m_model = std::make_shared<Model>(device, physicalDevice, commandPool, graphicsQueue, textureLayout, vertices, indices, std::move(texture));
}

} // namespace VulkanHelpers
