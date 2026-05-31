#ifndef ORDER_SYSTEM_H
#define ORDER_SYSTEM_H

#include <entt/entt.hpp>

#include "pathfinder.h"

namespace Systems {

void processOrders(entt::registry &registry, float deltaTime, const Pathfinder &pathfinder);

} // namespace Systems

#endif // ORDER_SYSTEM_H
