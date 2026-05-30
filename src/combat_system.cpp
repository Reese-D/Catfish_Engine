#include <vector>

#include <glm/glm.hpp>

#include "combat_system.h"
#include "components.h"
#include "orders.h"

namespace Systems {

void processCombat(entt::registry &registry, float deltaTime) {
    // Tick all combat timers up to cooldown
    for (auto entity : registry.view<Components::Combat>()) {
        auto &c = registry.get<Components::Combat>(entity);
        if (c.timer < c.cooldown) c.timer += deltaTime;
    }

    std::vector<entt::entity> toDestroy;

    auto view = registry.view<Components::OrderQueue, Components::Transform,
                              Components::Combat, Components::MovementSpeed>();
    for (auto entity : view) {
        auto &queue = view.get<Components::OrderQueue>(entity);
        if (queue.empty()) continue;

        auto *attack = std::get_if<Orders::AttackOrder>(&queue.orders.front());
        if (!attack) continue;

        // If target no longer alive, drop the order
        if (!registry.valid(attack->target) ||
            !registry.all_of<Components::Health>(attack->target)) {
            queue.orders.pop_front();
            continue;
        }

        auto       &selfTransform   = view.get<Components::Transform>(entity);
        auto       &combat          = view.get<Components::Combat>(entity);
        const auto &speed           = view.get<Components::MovementSpeed>(entity);
        const auto &targetPos       = registry.get<Components::Transform>(attack->target).position;

        // Use XY distance only so Z differences don't affect range/movement
        glm::vec2 delta2d{targetPos.x - selfTransform.position.x,
                          targetPos.y - selfTransform.position.y};
        float dist2d = glm::length(delta2d);

        if (dist2d > combat.range) {
            // Move toward target
            glm::vec3 dir{delta2d.x, delta2d.y, 0.0f};
            selfTransform.position += glm::normalize(dir) * speed.speed * deltaTime;
        } else if (combat.timer >= combat.cooldown) {
            // Attack
            auto &health = registry.get<Components::Health>(attack->target);
            health.current -= combat.damage;
            combat.timer = 0.0f;

            if (health.current <= 0.0f) {
                toDestroy.push_back(attack->target);
                queue.orders.pop_front();
            }
        }
    }

    for (auto dead : toDestroy) {
        if (registry.valid(dead)) registry.destroy(dead);
    }
}

} // namespace Systems
