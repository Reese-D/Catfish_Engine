#ifndef MOVEMENT_SYSTEM_H
#define MOVEMENT_SYSTEM_H

#include <entt/entt.hpp>

namespace Systems {

// Pushes overlapping units apart so they don't stack on the same position.
void applySeparation(entt::registry &registry);

} // namespace Systems

#endif // MOVEMENT_SYSTEM_H
