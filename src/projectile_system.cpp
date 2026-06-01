#include "projectile_system.h"

#include <cmath>
#include <numbers>
#include <vector>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "components.h"
#include "texture_image.h"
#include "vertex_buffer.h"

namespace Systems {

// ---- model creation --------------------------------------------------------

std::shared_ptr<VulkanHelpers::Model> createProjectileModel(
    const vk::raii::Device &device,
    const vk::raii::PhysicalDevice &physicalDevice,
    const vk::raii::CommandPool &commandPool,
    const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
) {
    constexpr int   N = 20;
    constexpr float R = 0.25f;

    // Bright cyan 1×1 texture
    std::array<unsigned char, 4> px = {0, 220, 255, 255};
    auto tex = std::make_shared<VulkanHelpers::TextureImage>(
        device, physicalDevice, commandPool, graphicsQueue,
        px.data(), 1, 1
    );

    std::vector<VulkanHelpers::Vertex> verts;
    verts.reserve(N + 1);

    // Centre vertex
    VulkanHelpers::Vertex centre{};
    centre.pos[0] = 0.0f; centre.pos[1] = 0.0f; centre.pos[2] = 0.0f;
    centre.color[0] = centre.color[1] = centre.color[2] = 1.0f;
    centre.texCoord[0] = 0.5f; centre.texCoord[1] = 0.5f;
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

    return std::make_shared<VulkanHelpers::Model>(
        device, physicalDevice, commandPool, graphicsQueue,
        textureLayout, verts, indices, std::move(tex)
    );
}

// ---- projectile spawning ---------------------------------------------------

void spawnProjectile(
    entt::registry &registry,
    std::shared_ptr<VulkanHelpers::Model> model,
    glm::vec3 origin,
    glm::vec3 velocity,
    Components::FactionId ownerFaction,
    float knockbackForce,
    float hitRadius,
    float lifetime
) {
    auto e = registry.create();
    registry.emplace<Components::Transform>(e, Components::Transform{
        .position = {origin.x, origin.y, 0.1f},
        .rotation = glm::quat{1.0f, 0.0f, 0.0f, 0.0f},
        .scale    = {1.0f, 1.0f, 1.0f},
    });
    registry.emplace<Components::RenderMesh>(e, Components::RenderMesh{std::move(model)});
    registry.emplace<Components::Projectile>(e, Components::Projectile{
        .ownerFaction  = ownerFaction,
        .velocity      = velocity,
        .knockbackForce = knockbackForce,
        .hitRadius     = hitRadius,
        .lifetime      = lifetime,
    });
}

// ---- per-frame updates -----------------------------------------------------

void tickAbilities(entt::registry &registry, float dt) {
    for (auto entity : registry.view<Components::Ability>()) {
        auto &ab = registry.get<Components::Ability>(entity);
        if (ab.timer < ab.cooldown)
            ab.timer += dt;
    }
}

void updateProjectiles(entt::registry &registry, float dt) {
    constexpr float WORLD_LIMIT = 25.0f;

    std::vector<entt::entity> toDestroy;

    for (auto projEntity : registry.view<Components::Projectile, Components::Transform>()) {
        auto &proj = registry.get<Components::Projectile>(projEntity);
        auto &t    = registry.get<Components::Transform>(projEntity);

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

        // Collision with enemy units
        bool hit = false;
        for (auto unitEntity : registry.view<Components::Transform, Components::Faction, Components::MovementSpeed>()) {
            if (hit) break;
            const auto &faction = registry.get<Components::Faction>(unitEntity);
            if (faction.id == proj.ownerFaction) continue;

            const auto &ut  = registry.get<Components::Transform>(unitEntity);
            float dx = t.position.x - ut.position.x;
            float dy = t.position.y - ut.position.y;
            if (dx * dx + dy * dy > proj.hitRadius * proj.hitRadius) continue;

            // Apply knockback in the projectile's travel direction
            glm::vec3 dir = glm::length(proj.velocity) > 0.001f
                                ? glm::normalize(proj.velocity)
                                : glm::vec3{1, 0, 0};
            glm::vec2 push = glm::vec2(dir.x, dir.y) * proj.knockbackForce;

            if (registry.all_of<Components::Knockback>(unitEntity)) {
                registry.get<Components::Knockback>(unitEntity).force += push;
            } else {
                registry.emplace<Components::Knockback>(unitEntity, Components::Knockback{push, 5.0f});
            }

            toDestroy.push_back(projEntity);
            hit = true;
        }
    }

    for (auto e : toDestroy) {
        if (registry.valid(e))
            registry.destroy(e);
    }
}

// ---- ability input ----------------------------------------------------------

namespace {

glm::vec3 screenToRay(glm::vec2 mousePos, glm::vec2 screenSize, const Components::Camera &cam) {
    float ndcX = (2.0f * mousePos.x) / screenSize.x - 1.0f;
    float ndcY = (2.0f * mousePos.y) / screenSize.y - 1.0f;
    glm::mat4 invVP = glm::inverse(cam.proj * cam.view);
    glm::vec4 nearW = invVP * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 farW  = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    nearW /= nearW.w;
    farW  /= farW.w;
    return glm::normalize(glm::vec3(farW) - glm::vec3(nearW));
}

std::optional<glm::vec3> rayGroundHit(glm::vec3 origin, glm::vec3 dir) {
    if (std::abs(dir.z) < 1e-6f) return std::nullopt;
    float t = -origin.z / dir.z;
    if (t < 0.0f) return std::nullopt;
    return origin + t * dir;
}

} // namespace

void processAbilityInput(
    entt::registry &registry,
    const VulkanHelpers::Window &window,
    vk::Extent2D extent,
    std::shared_ptr<VulkanHelpers::Model> projectileModel
) {
    static bool prevQ = false;
    bool qDown = window.isKeyPressed(GLFW_KEY_Q);
    bool qJust = qDown && !prevQ;
    prevQ = qDown;

    if (!qJust) return;

    const Components::Camera *cam = nullptr;
    for (auto entity : registry.view<Components::Camera>())  {
        cam = &registry.get<Components::Camera>(entity);
        break;
    }
    if (!cam) return;

    auto [mx, my] = window.getMousePosition();
    glm::vec2 screenSize{static_cast<float>(extent.width), static_cast<float>(extent.height)};
    glm::vec3 rayDir  = screenToRay({static_cast<float>(mx), static_cast<float>(my)}, screenSize, *cam);
    auto      ground  = rayGroundHit(cam->position, rayDir);
    if (!ground) return;

    for (auto entity : registry.view<Components::Selected, Components::Ability,
                                     Components::Transform, Components::Faction>()) {
        auto &ab = registry.get<Components::Ability>(entity);
        if (ab.timer < ab.cooldown) continue;

        const auto &t       = registry.get<Components::Transform>(entity);
        const auto &faction = registry.get<Components::Faction>(entity);

        glm::vec3 delta = *ground - t.position;
        delta.z = 0.0f;
        float len = glm::length(delta);
        if (len < 0.001f) continue;

        glm::vec3 velocity = (delta / len) * ab.projectileSpeed;

        spawnProjectile(
            registry, projectileModel,
            t.position, velocity,
            faction.id, ab.knockbackForce
        );
        ab.timer = 0.0f;
    }
}

} // namespace Systems
