#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include "projectile_render.h"

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "components.h"
#include "texture_image.h"
#include "vertex_buffer.h"

namespace Systems {

namespace {

std::shared_ptr<VulkanHelpers::Model> makeDiscModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout, float radius, std::array<unsigned char, 4> pixelColor
) {
    constexpr int kN = 24;

    auto tex = std::make_shared<VulkanHelpers::TextureImage>(device, physicalDevice, commandPool, graphicsQueue, pixelColor.data(), 1, 1);

    std::vector<VulkanHelpers::Vertex> verts;
    verts.reserve(kN + 1);

    VulkanHelpers::Vertex centre{};
    centre.pos[0] = centre.pos[1] = centre.pos[2] = 0.0f;
    centre.color[0] = centre.color[1] = centre.color[2] = 1.0f;
    centre.texCoord[0] = centre.texCoord[1] = 0.5f;
    verts.push_back(centre);

    for (int i = 0; i < kN; ++i) {
        float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(kN);
        VulkanHelpers::Vertex v{};
        v.pos[0] = radius * std::cos(angle);
        v.pos[1] = radius * std::sin(angle);
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

// Flat quad, halfW along X (bolt direction) and halfH along Y (perpendicular).
// UV: uv.x = [0,1] across Y (cross-axis), uv.y = [0,1] across X (along-axis).
std::shared_ptr<VulkanHelpers::Model> makeQuadModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout, float halfW, float halfH, std::array<unsigned char, 4> pixelColor
) {
    auto tex = std::make_shared<VulkanHelpers::TextureImage>(device, physicalDevice, commandPool, graphicsQueue, pixelColor.data(), 1, 1);

    std::vector<VulkanHelpers::Vertex> verts(4);
    // tail-bottom: x=-halfW, y=-halfH  →  uv.x(cross)=0, uv.y(along)=0
    verts[0].pos[0] = -halfW; verts[0].pos[1] = -halfH; verts[0].pos[2] = 0.0f;
    verts[0].color[0] = verts[0].color[1] = verts[0].color[2] = 1.0f;
    verts[0].texCoord[0] = 0.0f; verts[0].texCoord[1] = 0.0f;
    // tip-bottom: x=+halfW, y=-halfH  →  uv(0, 1)
    verts[1].pos[0] = +halfW; verts[1].pos[1] = -halfH; verts[1].pos[2] = 0.0f;
    verts[1].color[0] = verts[1].color[1] = verts[1].color[2] = 1.0f;
    verts[1].texCoord[0] = 0.0f; verts[1].texCoord[1] = 1.0f;
    // tip-top: x=+halfW, y=+halfH  →  uv(1, 1)
    verts[2].pos[0] = +halfW; verts[2].pos[1] = +halfH; verts[2].pos[2] = 0.0f;
    verts[2].color[0] = verts[2].color[1] = verts[2].color[2] = 1.0f;
    verts[2].texCoord[0] = 1.0f; verts[2].texCoord[1] = 1.0f;
    // tail-top: x=-halfW, y=+halfH  →  uv(1, 0)
    verts[3].pos[0] = -halfW; verts[3].pos[1] = +halfH; verts[3].pos[2] = 0.0f;
    verts[3].color[0] = verts[3].color[1] = verts[3].color[2] = 1.0f;
    verts[3].texCoord[0] = 1.0f; verts[3].texCoord[1] = 0.0f;

    std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};
    return std::make_shared<VulkanHelpers::Model>(device, physicalDevice, commandPool, graphicsQueue, textureLayout, verts, indices, std::move(tex));
}

} // namespace

std::shared_ptr<VulkanHelpers::Model> createProjectileModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
) {
    // White 1×1 pixel; the fireball shader ignores the texture colour.
    std::array<unsigned char, 4> px = {255, 255, 255, 255};
    return makeDiscModel(device, physicalDevice, commandPool, graphicsQueue, textureLayout, 0.25f, px);
}

std::shared_ptr<VulkanHelpers::Model> createGravityWellModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
) {
    // Larger disc so the black hole has more visual presence.
    std::array<unsigned char, 4> px = {255, 255, 255, 255};
    return makeDiscModel(device, physicalDevice, commandPool, graphicsQueue, textureLayout, 0.5f, px);
}

std::shared_ptr<VulkanHelpers::Model> createLightningModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
) {
    std::array<unsigned char, 4> px = {255, 255, 255, 255};
    return makeQuadModel(device, physicalDevice, commandPool, graphicsQueue, textureLayout, 0.5f, 0.075f, px);
}

std::shared_ptr<VulkanHelpers::Model> createChainModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
) {
    // 1-unit long × 0.1-unit wide; X-scaled at draw time to span between endpoints.
    std::array<unsigned char, 4> px = {255, 255, 255, 255};
    return makeQuadModel(device, physicalDevice, commandPool, graphicsQueue, textureLayout, 0.5f, 0.05f, px);
}

void appendProjectileDrawCalls(
    entt::registry &registry, std::vector<VulkanHelpers::ProjectileDrawCall> &draws, const VulkanHelpers::Model &fireballModel,
    const VulkanHelpers::Model &gravityWellModel, const VulkanHelpers::Model &lightningModel,
    const VulkanHelpers::Model &chainModel, float time
) {
    auto findByNetId = [&](uint32_t netId) -> entt::entity {
        for (auto e : registry.view<Components::NetworkId>()) {
            if (registry.get<Components::NetworkId>(e).id == netId)
                return e;
        }
        return entt::null;
    };

    for (auto entity : registry.view<Components::Transform, Components::Projectile>()) {
        const auto &t = registry.get<Components::Transform>(entity);

        if (registry.all_of<Components::ChainLink>(entity)) {
            const auto &cl = registry.get<Components::ChainLink>(entity);
            auto anchorEnt = findByNetId(cl.anchorNetId);
            auto hitEnt    = findByNetId(cl.hitNetId);
            if (anchorEnt == entt::null || hitEnt == entt::null)
                continue;
            glm::vec3 posA = registry.get<Components::Transform>(anchorEnt).position;
            glm::vec3 posB = registry.get<Components::Transform>(hitEnt).position;
            glm::vec2 delta{posB.x - posA.x, posB.y - posA.y};
            float len = glm::length(delta);
            if (len < 0.01f)
                continue;
            float angle = std::atan2(delta.y, delta.x);
            glm::vec3 mid{(posA.x + posB.x) * 0.5f, (posA.y + posB.y) * 0.5f, 0.05f};
            glm::mat4 mm = glm::translate(glm::mat4(1.0f), mid);
            mm *= glm::rotate(glm::mat4(1.0f), angle, glm::vec3{0.0f, 0.0f, 1.0f});
            mm  = glm::scale(mm, glm::vec3{len, 1.0f, 1.0f});
            draws.push_back(VulkanHelpers::ProjectileDrawCall{
                .model = &chainModel,
                .materialSet = chainModel.getMaterial().getDescriptorSet(),
                .transform = mm,
                .time = time,
                .shaderType = 4u,
            });
            continue;
        }

        bool isGravityWell  = registry.all_of<Components::GravityWell>(entity);
        bool isLightning    = registry.all_of<Components::LightningBolt>(entity);
        bool isChainFlying  = registry.all_of<Components::ChainProjectile>(entity);

        const VulkanHelpers::Model &model = isGravityWell ? gravityWellModel
                                          : isLightning   ? lightningModel
                                          : fireballModel;

        glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), t.position);
        if (isLightning)
            modelMatrix *= glm::mat4_cast(t.rotation);
        modelMatrix = glm::scale(modelMatrix, t.scale);

        uint32_t shaderType = isGravityWell ? 1u
                            : isLightning   ? 2u
                            : isChainFlying ? 3u
                            : 0u;
        draws.push_back(VulkanHelpers::ProjectileDrawCall{
            .model = &model,
            .materialSet = model.getMaterial().getDescriptorSet(),
            .transform = modelMatrix,
            .time = time,
            .shaderType = shaderType,
        });
    }
}

} // namespace Systems
