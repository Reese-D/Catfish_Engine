#ifndef ORDER_SYSTEM_H
#define ORDER_SYSTEM_H

#include <entt/entt.hpp>

namespace Systems {

void processOrders(entt::registry &registry, float deltaTime);

} // namespace Systems

#endif // ORDER_SYSTEM_H
