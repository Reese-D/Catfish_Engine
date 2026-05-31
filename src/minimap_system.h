#ifndef MINIMAP_SYSTEM_H
#define MINIMAP_SYSTEM_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#include <entt/entt.hpp>

#include "fog_of_war.h"

namespace Systems {

// Draws a fixed minimap overlay in the bottom-right corner.
// Modifies Camera component on click-to-pan.
void drawMinimap(const FogOfWar &fog, entt::registry &registry, vk::Extent2D extent);

} // namespace Systems

#endif // MINIMAP_SYSTEM_H
