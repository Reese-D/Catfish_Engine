#include "lava_system.h"

#include <array>

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
    // Deep orange-red, fully opaque
    std::array<unsigned char, 4> px = {220, 55, 0, 255};
    auto tex = std::make_shared<VulkanHelpers::TextureImage>(device, physicalDevice, commandPool, graphicsQueue, px.data(), 1, 1);

    // 1×1 quad centred at origin in XY plane, CCW winding from +Z
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
    constexpr float WORLD_MIN = -20.0f;
    constexpr float WORLD_MAX = 20.0f;
    constexpr float CELL = 1.0f;
    constexpr float Z = 0.02f; // above terrain, below unit bodies

    vk::DescriptorSet matSet = tileModel.getMaterial().getDescriptorSet();

    for (float cy = WORLD_MIN; cy < WORLD_MAX; cy += CELL) {
        for (float cx = WORLD_MIN; cx < WORLD_MAX; cx += CELL) {
            float midX = cx + CELL * 0.5f;
            float midY = cy + CELL * 0.5f;
            if (!lava.isLava({midX, midY}))
                continue;

            glm::mat4 m = glm::translate(glm::mat4(1.0f), {midX, midY, Z});
            draws.push_back({&tileModel, matSet, m});
        }
    }
}

void applyLavaDamage(const LavaZone &lava, entt::registry &registry, float dt) {
    float dmg = lava.getDamagePerSecond() * dt;
    for (auto entity : registry.view<Components::Transform, Components::Health, Components::MovementSpeed>()) {
        const auto &t = registry.get<Components::Transform>(entity);
        if (!lava.isLava({t.position.x, t.position.y}))
            continue;
        auto &health = registry.get<Components::Health>(entity);
        health.current = glm::max(0.0f, health.current - dmg);
    }
}

} // namespace Systems
