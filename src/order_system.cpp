#include <glm/glm.hpp>

#include "components.h"
#include "order_system.h"
#include "orders.h"

namespace Systems {

void processOrders(entt::registry &registry, float deltaTime) {
    auto view = registry.view<Components::OrderQueue, Components::Transform, Components::MovementSpeed>();

    for (auto entity : view) {
        auto &queue = view.get<Components::OrderQueue>(entity);
        if (queue.empty()) continue;

        auto &transform = view.get<Components::Transform>(entity);
        const auto &speed = view.get<Components::MovementSpeed>(entity);

        std::visit(
            Orders::overloaded{
                [&](Orders::MoveOrder &move) {
                    glm::vec3 delta = move.destination - transform.position;
                    float distance = glm::length(delta);
                    if (distance < 0.05f) {
                        queue.orders.pop_front();
                        return;
                    }
                    transform.position += glm::normalize(delta) * speed.speed * deltaTime;
                },
                [&](Orders::AttackOrder &) {
                    // stub — combat not yet implemented
                },
                [&](Orders::HoldOrder &) {
                    // intentionally idle
                },
            },
            queue.orders.front()
        );
    }
}

} // namespace Systems
