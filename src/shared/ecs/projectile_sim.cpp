#include "projectile_sim.h"

#include <cmath>
#include <vector>

#include <glm/glm.hpp>

#include "components.h"

namespace Systems {

entt::entity spawnProjectile(
    entt::registry &registry, glm::vec3 origin, glm::vec3 velocity,
    Components::FactionId ownerFaction, float knockbackForce,
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
            bool hit = false;
            for (auto unitEntity : registry.view<Components::Transform, Components::MovementSpeed, Components::Velocity>()) {
                if (hit)
                    break;
                const auto &ut = registry.get<Components::Transform>(unitEntity);
                float dx = t.position.x - ut.position.x;
                float dy = t.position.y - ut.position.y;
                if (dx * dx + dy * dy > proj.hitRadius * proj.hitRadius)
                    continue;

                glm::vec3 d = glm::length(proj.velocity) > 0.001f ? glm::normalize(proj.velocity) : glm::vec3{1, 0, 0};
                registry.get<Components::Velocity>(unitEntity).vel += glm::vec2(d.x, d.y) * proj.knockbackForce;

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
