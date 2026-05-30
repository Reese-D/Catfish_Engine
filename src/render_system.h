#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <vector>

#include <entt/entt.hpp>

#include "draw_call.h"
#include "model.h"

namespace Systems {

std::vector<VulkanHelpers::DrawCall> collectDrawCalls(entt::registry &registry);

// Appends one ring draw call per Selected entity using a shared ring model.
void appendSelectionRings(
    entt::registry &registry,
    std::vector<VulkanHelpers::DrawCall> &draws,
    const VulkanHelpers::Model &ringModel
);

} // namespace Systems

#endif // RENDER_SYSTEM_H
