#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include "lava_render.h"

#include <array>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "components.h"
#include "texture_image.h"
#include "vertex_buffer.h"

namespace Systems {

std::shared_ptr<VulkanHelpers::Model> createLavaTileModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
) {
    std::array<unsigned char, 4> px = {220, 55, 0, 255};
    auto tex = std::make_shared<VulkanHelpers::TextureImage>(device, physicalDevice, commandPool, graphicsQueue, px.data(), 1, 1);

    std::vector<VulkanHelpers::Vertex> verts(4);
    verts[0].pos[0] = -0.5f;
    verts[0].pos[1] = -0.5f;
    verts[0].pos[2] = 0.0f;
    verts[0].texCoord[0] = 0.0f;
    verts[0].texCoord[1] = 1.0f;
    verts[1].pos[0] = 0.5f;
    verts[1].pos[1] = -0.5f;
    verts[1].pos[2] = 0.0f;
    verts[1].texCoord[0] = 1.0f;
    verts[1].texCoord[1] = 1.0f;
    verts[2].pos[0] = 0.5f;
    verts[2].pos[1] = 0.5f;
    verts[2].pos[2] = 0.0f;
    verts[2].texCoord[0] = 1.0f;
    verts[2].texCoord[1] = 0.0f;
    verts[3].pos[0] = -0.5f;
    verts[3].pos[1] = 0.5f;
    verts[3].pos[2] = 0.0f;
    verts[3].texCoord[0] = 0.0f;
    verts[3].texCoord[1] = 0.0f;

    for (auto &v : verts)
        v.color[0] = v.color[1] = v.color[2] = 1.0f;

    std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};
    return std::make_shared<VulkanHelpers::Model>(device, physicalDevice, commandPool, graphicsQueue, textureLayout, verts, indices, std::move(tex));
}

void appendLavaDrawCalls(const LavaZone &lava, std::vector<VulkanHelpers::DrawCall> &draws, const VulkanHelpers::Model &tileModel) {
    constexpr float kWorldMin = WorldBounds::kMin;
    constexpr float kWorldMax = WorldBounds::kMax;
    constexpr float kCell = 1.0f;
    constexpr float kZ = 0.02f;

    vk::DescriptorSet matSet = tileModel.getMaterial().getDescriptorSet();

    for (float cy = kWorldMin; cy < kWorldMax; cy += kCell) {
        for (float cx = kWorldMin; cx < kWorldMax; cx += kCell) {
            float midX = cx + kCell * 0.5f;
            float midY = cy + kCell * 0.5f;
            if (!lava.isLava({midX, midY}))
                continue;
            glm::mat4 m = glm::translate(glm::mat4(1.0f), {midX, midY, kZ});
            draws.push_back({&tileModel, matSet, m});
        }
    }
}

} // namespace Systems
