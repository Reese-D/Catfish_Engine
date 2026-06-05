#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include "projectile_render.h"

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

#include "texture_image.h"
#include "vertex_buffer.h"

namespace Systems {

std::shared_ptr<VulkanHelpers::Model> createProjectileModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
) {
    constexpr int kN = 20;
    constexpr float kR = 0.25f;

    std::array<unsigned char, 4> px = {0, 220, 255, 255};
    auto tex = std::make_shared<VulkanHelpers::TextureImage>(device, physicalDevice, commandPool, graphicsQueue, px.data(), 1, 1);

    std::vector<VulkanHelpers::Vertex> verts;
    verts.reserve(kN + 1);

    VulkanHelpers::Vertex centre{};
    centre.pos[0] = 0.0f;
    centre.pos[1] = 0.0f;
    centre.pos[2] = 0.0f;
    centre.color[0] = centre.color[1] = centre.color[2] = 1.0f;
    centre.texCoord[0] = 0.5f;
    centre.texCoord[1] = 0.5f;
    verts.push_back(centre);

    for (int i = 0; i < kN; ++i) {
        float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(kN);
        VulkanHelpers::Vertex v{};
        v.pos[0] = kR * std::cos(angle);
        v.pos[1] = kR * std::sin(angle);
        v.pos[2] = 0.0f;
        v.color[0] = v.color[1] = v.color[2] = 1.0f;
        v.texCoord[0] = 0.5f + 0.5f * std::cos(angle);
        v.texCoord[1] = 0.5f + 0.5f * std::sin(angle);
        verts.push_back(v);
    }

    std::vector<uint32_t> indices;
    indices.reserve(kN * 3);
    for (int i = 0; i < kN; ++i) {
        indices.push_back(0);
        indices.push_back(static_cast<uint32_t>(i + 1));
        indices.push_back(static_cast<uint32_t>((i + 1) % kN + 1));
    }

    return std::make_shared<VulkanHelpers::Model>(device, physicalDevice, commandPool, graphicsQueue, textureLayout, verts, indices, std::move(tex));
}

} // namespace Systems
