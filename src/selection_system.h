#ifndef SELECTION_SYSTEM_H
#define SELECTION_SYSTEM_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#include <entt/entt.hpp>

#include "spatial_grid.h"
#include "window.h"

namespace Systems {

void updateSelection(
    entt::registry &registry,
    const VulkanHelpers::Window &window,
    vk::Extent2D extent,
    const VulkanHelpers::SpatialGrid &grid
);

} // namespace Systems

#endif // SELECTION_SYSTEM_H
