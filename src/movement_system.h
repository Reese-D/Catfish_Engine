#ifndef MOVEMENT_SYSTEM_H
#define MOVEMENT_SYSTEM_H

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace Systems {

// Pushes overlapping units apart so they don't stack on the same position.
void applySeparation(entt::registry &registry);

// Clamps all unit positions (entities with MovementSpeed) to [worldMin, worldMax] on XY.
void clampToBounds(entt::registry &registry, glm::vec2 worldMin, glm::vec2 worldMax);

} // namespace Systems

#endif // MOVEMENT_SYSTEM_H
