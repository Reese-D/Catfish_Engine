#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "components.h"
#include "movement_system.h"

namespace Systems {

void applySeparation(entt::registry &registry) {
    constexpr float minDist = 0.8f;  // minimum separation between unit centres
    constexpr float factor  = 0.35f; // fraction of overlap resolved per frame

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
            float     dist  = glm::length(delta);
            if (dist > 0.001f && dist < minDist) {
                glm::vec2 push = glm::normalize(delta) * (minDist - dist) * factor;
                forces[i] += push;
                forces[j] -= push;
            }
        }
    }

    for (std::size_t i = 0; i < snapshot.size(); ++i) {
        if (glm::length(forces[i]) > 0.0f) {
            auto &t     = registry.get<Components::Transform>(snapshot[i].first);
            t.position.x += forces[i].x;
            t.position.y += forces[i].y;
        }
    }
}

} // namespace Systems
