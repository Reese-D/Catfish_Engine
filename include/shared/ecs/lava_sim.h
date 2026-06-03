#ifndef LAVA_SIM_H
#define LAVA_SIM_H

#include <entt/entt.hpp>

#include "lava_zone.h"

namespace Systems {

// Applies continuous HP drain to every unit in lava.
void applyLavaDamage(const LavaZone &lava, entt::registry &registry, float dt);

} // namespace Systems

#endif // LAVA_SIM_H
