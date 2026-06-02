#include <glm/glm.hpp>

#include "components.h"
#include "order_system.h"
#include "orders.h"
#include "pathfinder.h"

namespace Systems {

void processOrders(entt::registry &registry, float deltaTime, const Pathfinder *pathfinder) {
    auto view = registry.view<Components::OrderQueue, Components::Transform, Components::MovementSpeed>();

    for (auto entity : view) {
        auto &queue = view.get<Components::OrderQueue>(entity);
        if (queue.empty())
            continue;

        auto &transform = view.get<Components::Transform>(entity);
        const auto &speed = view.get<Components::MovementSpeed>(entity);

        std::visit(
            Orders::overloaded{
                [&](Orders::MoveOrder &move) {
                    if (move.path.empty()) {
                        if (pathfinder) {
                            move.path = pathfinder->findPath({transform.position.x, transform.position.y}, {move.destination.x, move.destination.y});
                        }
                        move.pathIndex = 0;
                        if (move.path.empty())
                            move.path.push_back(move.destination);
                    }

                    const glm::vec3 &waypoint = move.path[move.pathIndex];
                    glm::vec2 delta2d{waypoint.x - transform.position.x, waypoint.y - transform.position.y};
                    float dist = glm::length(delta2d);

                    if (dist < 0.05f) {
                        if (++move.pathIndex >= move.path.size())
                            queue.orders.pop_front();
                        return;
                    }

                    glm::vec3 dir{delta2d.x, delta2d.y, 0.0f};
                    transform.position += glm::normalize(dir) * speed.speed * deltaTime;
                },
                [&](Orders::AttackOrder &) {
                    // handled by CombatSystem
                },
                [&](Orders::HoldOrder &) {},
            },
            queue.orders.front()
        );
    }
}

} // namespace Systems
