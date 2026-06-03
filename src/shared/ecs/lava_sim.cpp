#include "lava_sim.h"

#include <glm/glm.hpp>

#include "components.h"

namespace Systems {

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
