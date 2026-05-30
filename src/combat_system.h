#ifndef COMBAT_SYSTEM_H
#define COMBAT_SYSTEM_H

#include <entt/entt.hpp>

namespace Systems {

// Processes AttackOrders: moves units into range, applies damage, removes
// entities whose Health reaches zero.
void processCombat(entt::registry &registry, float deltaTime);

} // namespace Systems

#endif // COMBAT_SYSTEM_H
