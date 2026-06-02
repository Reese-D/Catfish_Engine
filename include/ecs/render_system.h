#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <vector>

#include <entt/entt.hpp>

#include "draw_call.h"
#include "fog_of_war.h"
#include "model.h"

namespace Systems {

// fog may be nullptr to disable culling.
std::vector<VulkanHelpers::DrawCall> collectDrawCalls(entt::registry &registry, const FogOfWar *fog = nullptr);

// Appends one ring draw call per Selected entity using a shared ring model.
void appendSelectionRings(entt::registry &registry, std::vector<VulkanHelpers::DrawCall> &draws, const VulkanHelpers::Model &ringModel, const FogOfWar *fog = nullptr);

} // namespace Systems

#endif // RENDER_SYSTEM_H
