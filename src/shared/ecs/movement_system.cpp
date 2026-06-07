#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "components.h"
#include "movement_system.h"

namespace Systems {

void applySeparation(entt::registry &registry) {
    constexpr float kMinDist = 0.8f;
    constexpr float kFactor = 0.35f;

    // --- Unit-unit separation ---
    std::vector<std::pair<entt::entity, glm::vec2>> units;
    for (auto entity : registry.view<Components::Transform, Components::Selectable>()) {
        const auto &t = registry.get<Components::Transform>(entity);
        units.emplace_back(entity, glm::vec2{t.position.x, t.position.y});
    }

    std::vector<glm::vec2> forces(units.size(), glm::vec2{0.0f});
    for (std::size_t i = 0; i < units.size(); ++i) {
        for (std::size_t j = i + 1; j < units.size(); ++j) {
            glm::vec2 delta = units[i].second - units[j].second;
            float dist = glm::length(delta);
            if (dist > 0.001f && dist < kMinDist) {
                glm::vec2 push = glm::normalize(delta) * (kMinDist - dist) * kFactor;
                forces[i] += push;
                forces[j] -= push;
            }
        }
    }
    for (std::size_t i = 0; i < units.size(); ++i) {
        if (glm::length(forces[i]) > 0.0f) {
            auto &t = registry.get<Components::Transform>(units[i].first);
            t.position.x += forces[i].x;
            t.position.y += forces[i].y;
        }
    }

    // --- Unit-rock separation (mass-weighted: unit takes most of the push) ---
    constexpr float kUnitMass = 1.0f;
    constexpr float kUnitRadius = kMinDist * 0.5f;
    for (auto rockEntity : registry.view<Components::Transform, Components::Rock>()) {
        auto &rt = registry.get<Components::Transform>(rockEntity);

        // Use Collider if present; fall back to scale-derived estimate.
        glm::vec2 colliderOffset{0.0f, 0.0f};
        float rockRadius = rt.scale.x * kUnitRadius;
        if (registry.all_of<Components::Collider>(rockEntity)) {
            const auto &col = registry.get<Components::Collider>(rockEntity);
            colliderOffset = col.offset;
            rockRadius = col.radius;
        }
        const float minSep = kUnitRadius + rockRadius;
        const float rockMass = registry.all_of<Components::Mass>(rockEntity)
                                   ? registry.get<Components::Mass>(rockEntity).value
                                   : kUnitMass;
        const float totalMass = kUnitMass + rockMass;

        for (auto unitEntity : registry.view<Components::Transform, Components::Selectable>()) {
            auto &ut = registry.get<Components::Transform>(unitEntity);
            glm::vec2 rockCenter{rt.position.x + colliderOffset.x, rt.position.y + colliderOffset.y};
            glm::vec2 delta{ut.position.x - rockCenter.x, ut.position.y - rockCenter.y};
            float dist = glm::length(delta);
            if (dist > 0.001f && dist < minSep) {
                glm::vec2 push = glm::normalize(delta) * (minSep - dist) * kFactor;
                ut.position.x += push.x * rockMass / totalMass;
                ut.position.y += push.y * rockMass / totalMass;
                rt.position.x -= push.x * kUnitMass / totalMass;
                rt.position.y -= push.y * kUnitMass / totalMass;
            }
        }
    }
}

void applyThrust(entt::registry &registry, float dt) {
    for (auto entity : registry.view<Components::ThrustDirection, Components::Velocity>()) {
        const auto &th = registry.get<Components::ThrustDirection>(entity);
        if (glm::length(th.dir) < 0.001f)
            continue;
        registry.get<Components::Velocity>(entity).vel += th.dir * (th.force * dt);
    }
}

void applyVelocity(entt::registry &registry, float dt) {
    for (auto entity : registry.view<Components::Velocity, Components::Transform>()) {
        auto &v = registry.get<Components::Velocity>(entity);
        auto &t = registry.get<Components::Transform>(entity);
        t.position.x += v.vel.x * dt;
        t.position.y += v.vel.y * dt;
    }
}

void clampToBounds(entt::registry &registry, glm::vec2 worldMin, glm::vec2 worldMax) {
    for (auto entity : registry.view<Components::Transform, Components::Velocity>()) {
        auto &t = registry.get<Components::Transform>(entity);
        bool hitX = t.position.x <= worldMin.x || t.position.x >= worldMax.x;
        bool hitY = t.position.y <= worldMin.y || t.position.y >= worldMax.y;
        t.position.x = glm::clamp(t.position.x, worldMin.x, worldMax.x);
        t.position.y = glm::clamp(t.position.y, worldMin.y, worldMax.y);
        if ((hitX || hitY) && registry.all_of<Components::Velocity>(entity)) {
            auto &v = registry.get<Components::Velocity>(entity);
            if (hitX)
                v.vel.x = 0.0f;
            if (hitY)
                v.vel.y = 0.0f;
        }
    }
}

void applyFriction(entt::registry &registry, float dt) {
    for (auto entity : registry.view<Components::Velocity, Components::Friction>()) {
        auto &v = registry.get<Components::Velocity>(entity);
        const float coeff = registry.get<Components::Friction>(entity).coefficient;
        float decay = 1.0f - coeff * dt;
        if (decay < 0.0f)
            decay = 0.0f;
        v.vel *= decay;
    }
}

} // namespace Systems
