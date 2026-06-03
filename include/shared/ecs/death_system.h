#ifndef DEATH_SYSTEM_H
#define DEATH_SYSTEM_H

#include <vector>

#include <entt/entt.hpp>

#include "components.h"

namespace Systems {

// Destroys every entity whose Health has reached zero.
// Returns the faction of each unit that died this frame so the caller
// can check for elimination conditions (e.g. to trigger a round end).
std::vector<Components::FactionId> processDeath(entt::registry &registry);

} // namespace Systems

#endif // DEATH_SYSTEM_H
