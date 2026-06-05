#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "components.h"
#include "movement_system.h"

namespace Systems {

void applySeparation(entt::registry &registry) {
    constexpr float kMinDist = 0.8f; // minimum separation between unit centres
    constexpr float kFactor = 0.35f; // fraction of overlap resolved per frame

    // Snapshot positions — we apply forces after all comparisons
    std::vector<std::pair<entt::entity, glm::vec2>> snapshot;
    for (auto entity : registry.view<Components::Transform, Components::Selectable>()) {
        const auto &t = registry.get<Components::Transform>(entity);
        snapshot.emplace_back(entity, glm::vec2{t.position.x, t.position.y});
    }

    std::vector<glm::vec2> forces(snapshot.size(), glm::vec2{0.0f});

    for (std::size_t i = 0; i < snapshot.size(); ++i) {
        for (std::size_t j = i + 1; j < snapshot.size(); ++j) {
            glm::vec2 delta = snapshot[i].second - snapshot[j].second;
            float dist = glm::length(delta);
            if (dist > 0.001f && dist < kMinDist) {
                glm::vec2 push = glm::normalize(delta) * (kMinDist - dist) * kFactor;
                forces[i] += push;
                forces[j] -= push;
            }
        }
    }

    for (std::size_t i = 0; i < snapshot.size(); ++i) {
        if (glm::length(forces[i]) > 0.0f) {
            auto &t = registry.get<Components::Transform>(snapshot[i].first);
            t.position.x += forces[i].x;
            t.position.y += forces[i].y;
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
    for (auto entity : registry.view<Components::Transform, Components::MovementSpeed>()) {
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

} // namespace Systems
