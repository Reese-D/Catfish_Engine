#ifndef PROJECTILE_SIM_H
#define PROJECTILE_SIM_H

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "components.h"

namespace Systems {

// Spawns a projectile entity with simulation components only (no RenderMesh).
// The client adds RenderMesh separately when it receives the entity via snapshot.
entt::entity
spawnProjectile(entt::registry &registry, glm::vec3 origin, glm::vec3 velocity, Components::FactionId ownerFaction, float knockbackForce, float hitRadius = 0.4f, float lifetime = 6.0f);

// Ticks AbilitySet cooldown timers.
void tickAbilities(entt::registry &registry, float dt);

// Moves projectiles, applies gravity-well pulls, checks collisions, despawns on hit or expiry.
void updateProjectiles(entt::registry &registry, float dt);

} // namespace Systems

#endif // PROJECTILE_SIM_H
