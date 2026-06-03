#include "projectile_system.h"

#include <cmath>
#include <numbers>
#include <vector>

#include <glm/glm.hpp>

#include "components.h"
#include "texture_image.h"
#include "vertex_buffer.h"

namespace Systems {

// ---- model creation --------------------------------------------------------

std::shared_ptr<VulkanHelpers::Model> createProjectileModel(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
) {
    constexpr int N = 20;
    constexpr float R = 0.25f;

    // Bright cyan 1×1 texture
    std::array<unsigned char, 4> px = {0, 220, 255, 255};
    auto tex = std::make_shared<VulkanHelpers::TextureImage>(device, physicalDevice, commandPool, graphicsQueue, px.data(), 1, 1);

    std::vector<VulkanHelpers::Vertex> verts;
    verts.reserve(N + 1);

    // Centre vertex
    VulkanHelpers::Vertex centre{};
    centre.pos[0] = 0.0f;
    centre.pos[1] = 0.0f;
    centre.pos[2] = 0.0f;
    centre.color[0] = centre.color[1] = centre.color[2] = 1.0f;
    centre.texCoord[0] = 0.5f;
    centre.texCoord[1] = 0.5f;
    verts.push_back(centre);

    for (int i = 0; i < N; ++i) {
        float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(N);
        VulkanHelpers::Vertex v{};
        v.pos[0] = R * std::cos(angle);
        v.pos[1] = R * std::sin(angle);
        v.pos[2] = 0.0f;
        v.color[0] = v.color[1] = v.color[2] = 1.0f;
        v.texCoord[0] = 0.5f + 0.5f * std::cos(angle);
        v.texCoord[1] = 0.5f + 0.5f * std::sin(angle);
        verts.push_back(v);
    }

    std::vector<uint32_t> indices;
    indices.reserve(N * 3);
    for (int i = 0; i < N; ++i) {
        indices.push_back(0);
        indices.push_back(static_cast<uint32_t>(i + 1));
        indices.push_back(static_cast<uint32_t>((i + 1) % N + 1));
    }

    return std::make_shared<VulkanHelpers::Model>(device, physicalDevice, commandPool, graphicsQueue, textureLayout, verts, indices, std::move(tex));
}

// ---- projectile spawning ---------------------------------------------------

entt::entity spawnProjectile(
    entt::registry &registry, std::shared_ptr<VulkanHelpers::Model> model, glm::vec3 origin, glm::vec3 velocity, Components::FactionId ownerFaction, float knockbackForce,
    float hitRadius, float lifetime
) {
    glm::vec3 dir = glm::length(velocity) > 0.001f ? glm::normalize(velocity) : glm::vec3{1, 0, 0};
    glm::vec3 spawnPos = origin + dir * (hitRadius + 0.1f);

    auto e = registry.create();
    registry.emplace<Components::Transform>(
        e, Components::Transform{
               .position = {spawnPos.x, spawnPos.y, 0.1f},
               .rotation = glm::quat{1.0f, 0.0f, 0.0f, 0.0f},
               .scale = {1.0f, 1.0f, 1.0f},
           }
    );
    registry.emplace<Components::RenderMesh>(e, Components::RenderMesh{std::move(model)});
    registry.emplace<Components::Projectile>(
        e, Components::Projectile{
               .ownerFaction = ownerFaction,
               .velocity = velocity,
               .knockbackForce = knockbackForce,
               .hitRadius = hitRadius,
               .lifetime = lifetime,
           }
    );
    return e;
}

// ---- per-frame updates -----------------------------------------------------

void tickAbilities(entt::registry &registry, float dt) {
    for (auto entity : registry.view<Components::AbilitySet>()) {
        for (auto &slot : registry.get<Components::AbilitySet>(entity).slots)
            if (slot.timer < slot.cooldown)
                slot.timer += dt;
    }
}

void updateProjectiles(entt::registry &registry, float dt) {
    constexpr float WORLD_LIMIT = 25.0f;

    std::vector<entt::entity> toDestroy;

    for (auto projEntity : registry.view<Components::Projectile, Components::Transform>()) {
        auto &proj = registry.get<Components::Projectile>(projEntity);
        auto &t = registry.get<Components::Transform>(projEntity);

        proj.lifetime -= dt;
        if (proj.lifetime <= 0.0f) {
            toDestroy.push_back(projEntity);
            continue;
        }

        t.position += proj.velocity * dt;

        if (std::abs(t.position.x) > WORLD_LIMIT || std::abs(t.position.y) > WORLD_LIMIT) {
            toDestroy.push_back(projEntity);
            continue;
        }

        if (registry.all_of<Components::GravityWell>(projEntity)) {
            auto &gw = registry.get<Components::GravityWell>(projEntity);
            if (gw.activationTimer < gw.activationDelay) {
                gw.activationTimer += dt;
            } else {
                // Inverse-square pull: acceleration = pullStrength / dist²
                for (auto unitEntity : registry.view<Components::Transform, Components::MovementSpeed, Components::Velocity>()) {
                    const auto &ut = registry.get<Components::Transform>(unitEntity);
                    glm::vec2 toWell{t.position.x - ut.position.x, t.position.y - ut.position.y};
                    float dist = glm::length(toWell);
                    if (dist < 0.1f || dist > gw.pullRadius)
                        continue;
                    float accelMag = gw.pullStrength / (dist * dist);
                    registry.get<Components::Velocity>(unitEntity).vel += glm::normalize(toWell) * (accelMag * dt);
                }
            }
        } else {
            // Regular projectile: flat impulse on first unit hit, then destroy.
            bool hit = false;
            for (auto unitEntity : registry.view<Components::Transform, Components::MovementSpeed, Components::Velocity>()) {
                if (hit)
                    break;
                const auto &ut = registry.get<Components::Transform>(unitEntity);
                float dx = t.position.x - ut.position.x;
                float dy = t.position.y - ut.position.y;
                if (dx * dx + dy * dy > proj.hitRadius * proj.hitRadius)
                    continue;

                glm::vec3 dir = glm::length(proj.velocity) > 0.001f ? glm::normalize(proj.velocity) : glm::vec3{1, 0, 0};
                registry.get<Components::Velocity>(unitEntity).vel += glm::vec2(dir.x, dir.y) * proj.knockbackForce;

                toDestroy.push_back(projEntity);
                hit = true;
            }
        }
    }

    for (auto e : toDestroy) {
        if (registry.valid(e))
            registry.destroy(e);
    }
}

} // namespace Systems
