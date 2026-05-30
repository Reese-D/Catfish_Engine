#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <vector>

#include <entt/entt.hpp>

#include "draw_call.h"

namespace Systems {

std::vector<VulkanHelpers::DrawCall> collectDrawCalls(entt::registry &registry);

} // namespace Systems

#endif // RENDER_SYSTEM_H
