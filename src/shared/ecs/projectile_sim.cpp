#include "projectile_sim.h"

#include <cmath>
#include <vector>

#include <glm/glm.hpp>

#include "components.h"

namespace Systems {

entt::entity spawnProjectile(entt::registry &registry, glm::vec3 origin, glm::vec3 velocity, Components::FactionId ownerFaction, float knockbackForce, float hitRadius, float lifetime) {
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
    constexpr float kWorldLimit = 25.0f;

    std::vector<entt::entity> toDestroy;

    // ---- Chain link tick: apply mutual pull, expire after duration ------------
    for (auto e : registry.view<Components::ChainLink>()) {
        auto &cl = registry.get<Components::ChainLink>(e);
        cl.timer += dt;
        if (cl.timer >= cl.duration || !registry.valid(cl.anchorEntity) || !registry.valid(cl.hitEntity)) {
            toDestroy.push_back(e);
            continue;
        }
        auto &ta = registry.get<Components::Transform>(cl.anchorEntity);
        auto &th = registry.get<Components::Transform>(cl.hitEntity);
        glm::vec2 toHit{th.position.x - ta.position.x, th.position.y - ta.position.y};
        float dist = glm::length(toHit);
        if (dist > 0.01f) {
            glm::vec2 dir = toHit / dist;
            auto massOf = [&](entt::entity ent) {
                return registry.all_of<Components::Mass>(ent) ? registry.get<Components::Mass>(ent).value : 1.0f;
            };
            if (registry.all_of<Components::Velocity>(cl.anchorEntity))
                registry.get<Components::Velocity>(cl.anchorEntity).vel += dir * (cl.pullStrength / massOf(cl.anchorEntity) * dt);
            if (registry.all_of<Components::Velocity>(cl.hitEntity))
                registry.get<Components::Velocity>(cl.hitEntity).vel += -dir * (cl.pullStrength / massOf(cl.hitEntity) * dt);
        }
        // Keep chain entity at midpoint for snapshot position
        registry.get<Components::Transform>(e).position = {
            (ta.position.x + th.position.x) * 0.5f,
            (ta.position.y + th.position.y) * 0.5f,
            0.05f,
        };
    }

    // ---- Struct to defer chain latching until after the main loop ------------
    struct LatchOp {
        entt::entity projEntity;
        entt::entity hitEntity;
    };
    std::vector<LatchOp> toLatch;

    for (auto projEntity : registry.view<Components::Projectile, Components::Transform>()) {
        if (registry.all_of<Components::ChainLink>(projEntity))
            continue; // handled above

        auto &proj = registry.get<Components::Projectile>(projEntity);
        auto &t = registry.get<Components::Transform>(projEntity);

        proj.lifetime -= dt;
        if (proj.lifetime <= 0.0f) {
            toDestroy.push_back(projEntity);
            continue;
        }

        t.position += proj.velocity * dt;

        if (std::abs(t.position.x) > kWorldLimit || std::abs(t.position.y) > kWorldLimit) {
            toDestroy.push_back(projEntity);
            continue;
        }

        auto massOf = [&](entt::entity e) {
            return registry.all_of<Components::Mass>(e) ? registry.get<Components::Mass>(e).value : 1.0f;
        };
        // Physics centre of a target entity, accounting for any Collider offset.
        auto collisionCenter = [&](entt::entity e) -> glm::vec2 {
            const auto &tr = registry.get<Components::Transform>(e);
            glm::vec2 c{tr.position.x, tr.position.y};
            if (registry.all_of<Components::Collider>(e))
                c += registry.get<Components::Collider>(e).offset;
            return c;
        };
        // Combined hit threshold: projectile radius + target's collider radius (0 for units).
        auto hitThreshold = [&](entt::entity target) -> float {
            if (registry.all_of<Components::Collider>(target))
                return proj.hitRadius + registry.get<Components::Collider>(target).radius;
            return proj.hitRadius;
        };

        if (registry.all_of<Components::ChainProjectile>(projEntity)) {
            auto &cp = registry.get<Components::ChainProjectile>(projEntity);
            for (auto unitEntity : registry.view<Components::Transform, Components::Velocity>()) {
                if (unitEntity == projEntity || unitEntity == cp.casterEntity)
                    continue;
                glm::vec2 center = collisionCenter(unitEntity);
                float dx = t.position.x - center.x;
                float dy = t.position.y - center.y;
                float threshold = hitThreshold(unitEntity);
                if (dx * dx + dy * dy > threshold * threshold)
                    continue;
                toLatch.push_back({projEntity, unitEntity});
                break;
            }
        } else if (registry.all_of<Components::GravityWell>(projEntity)) {
            auto &gw = registry.get<Components::GravityWell>(projEntity);
            if (gw.activationTimer < gw.activationDelay) {
                gw.activationTimer += dt;
            } else {
                for (auto unitEntity : registry.view<Components::Transform, Components::Velocity>()) {
                    glm::vec2 center = collisionCenter(unitEntity);
                    glm::vec2 toWell{t.position.x - center.x, t.position.y - center.y};
                    float dist = glm::length(toWell);
                    if (dist < 0.1f || dist > gw.pullRadius)
                        continue;
                    float accelMag = gw.pullStrength / (dist * dist * massOf(unitEntity));
                    registry.get<Components::Velocity>(unitEntity).vel += glm::normalize(toWell) * (accelMag * dt);
                }
            }
        } else if (registry.all_of<Components::LightningBolt>(projEntity)) {
            auto &lb = registry.get<Components::LightningBolt>(projEntity);
            if (lb.chargeTimer < lb.chargeDelay) {
                lb.chargeTimer += dt;
                if (lb.chargeTimer >= lb.chargeDelay)
                    proj.velocity = lb.boltDir * lb.speed;
                // no hit detection while charging
            } else {
                bool hit = false;
                for (auto unitEntity : registry.view<Components::Transform, Components::Velocity>()) {
                    if (hit)
                        break;
                    glm::vec2 center = collisionCenter(unitEntity);
                    float dx = t.position.x - center.x;
                    float dy = t.position.y - center.y;
                    float threshold = hitThreshold(unitEntity);
                    if (dx * dx + dy * dy > threshold * threshold)
                        continue;
                    registry.get<Components::Velocity>(unitEntity).vel +=
                        glm::vec2(lb.boltDir.x, lb.boltDir.y) * (proj.knockbackForce / massOf(unitEntity));
                    toDestroy.push_back(projEntity);
                    hit = true;
                }
            }
        } else {
            bool hit = false;
            for (auto unitEntity : registry.view<Components::Transform, Components::Velocity>()) {
                if (hit)
                    break;
                glm::vec2 center = collisionCenter(unitEntity);
                float dx = t.position.x - center.x;
                float dy = t.position.y - center.y;
                float threshold = hitThreshold(unitEntity);
                if (dx * dx + dy * dy > threshold * threshold)
                    continue;

                glm::vec3 d = glm::length(proj.velocity) > 0.001f ? glm::normalize(proj.velocity) : glm::vec3{1, 0, 0};
                registry.get<Components::Velocity>(unitEntity).vel +=
                    glm::vec2(d.x, d.y) * (proj.knockbackForce / massOf(unitEntity));

                toDestroy.push_back(projEntity);
                hit = true;
            }
        }
    }

    // ---- Apply chain latches ------------------------------------------------
    for (auto &op : toLatch) {
        if (!registry.valid(op.projEntity) || !registry.valid(op.hitEntity))
            continue;
        auto &proj = registry.get<Components::Projectile>(op.projEntity);
        auto &cp   = registry.get<Components::ChainProjectile>(op.projEntity);
        proj.velocity = {0.0f, 0.0f, 0.0f};
        uint32_t anchorNetId = registry.all_of<Components::NetworkId>(cp.casterEntity)
                                   ? registry.get<Components::NetworkId>(cp.casterEntity).id : 0u;
        uint32_t hitNetId    = registry.all_of<Components::NetworkId>(op.hitEntity)
                                   ? registry.get<Components::NetworkId>(op.hitEntity).id : 0u;
        registry.emplace<Components::ChainLink>(
            op.projEntity, Components::ChainLink{
                               .anchorEntity = cp.casterEntity,
                               .hitEntity    = op.hitEntity,
                               .anchorNetId  = anchorNetId,
                               .hitNetId     = hitNetId,
                               .pullStrength = cp.pullStrength,
                               .duration     = cp.pullDuration,
                           }
        );
        registry.remove<Components::ChainProjectile>(op.projEntity);
    }

    for (auto e : toDestroy) {
        if (registry.valid(e))
            registry.destroy(e);
    }
}

} // namespace Systems
