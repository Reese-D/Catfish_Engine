#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include "projectile_render.h"

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

void appendProjectileDrawCalls(
    entt::registry &registry, std::vector<VulkanHelpers::ProjectileDrawCall> &draws, const VulkanHelpers::Model &fireballModel, const VulkanHelpers::Model &gravityWellModel,
    float time
) {
    for (auto entity : registry.view<Components::Transform, Components::Projectile>()) {
        const auto &t = registry.get<Components::Transform>(entity);
        bool isGravityWell = registry.all_of<Components::GravityWell>(entity);

        const VulkanHelpers::Model &model = isGravityWell ? gravityWellModel : fireballModel;

        auto modelMatrix = glm::translate(glm::mat4(1.0f), t.position);
        modelMatrix = glm::scale(modelMatrix, t.scale);

        draws.push_back(VulkanHelpers::ProjectileDrawCall{
            .model = &model,
            .materialSet = model.getMaterial().getDescriptorSet(),
            .transform = modelMatrix,
            .time = time,
            .shaderType = isGravityWell ? 1u : 0u,
        });
    }
}

} // namespace Systems
