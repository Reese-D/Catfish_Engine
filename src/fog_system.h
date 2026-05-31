#ifndef FOG_SYSTEM_H
#define FOG_SYSTEM_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#include <entt/entt.hpp>

#include "fog_of_war.h"

namespace Systems {

// Projects each non-visible fog cell to screen space and draws a colored quad
// via ImGui's background draw list (renders above 3D output, below UI windows).
void drawFogOverlay(const FogOfWar &fog, entt::registry &registry, vk::Extent2D extent);

} // namespace Systems

#endif // FOG_SYSTEM_H
