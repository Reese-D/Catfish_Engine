#ifndef MOVEMENT_SYSTEM_H
#define MOVEMENT_SYSTEM_H

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace Systems {

// Pushes overlapping units apart so they don't stack on the same position.
void applySeparation(entt::registry &registry);

// Clamps all entities with Velocity to [worldMin, worldMax] on XY and zeroes velocity into walls.
void clampToBounds(entt::registry &registry, glm::vec2 worldMin, glm::vec2 worldMax);

// Moves all entities with Velocity by vel * dt each frame.
void applyVelocity(entt::registry &registry, float dt);

// Accelerates units in their current ThrustDirection each frame.
void applyThrust(entt::registry &registry, float dt);

// Decays velocity for all entities with a Friction component.
void applyFriction(entt::registry &registry, float dt);

} // namespace Systems

#endif // MOVEMENT_SYSTEM_H
