#ifndef CAMERA_SYSTEM_H
#define CAMERA_SYSTEM_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#include <entt/entt.hpp>

#include "uniform_buffer.h"

namespace Systems {

void updateCamera(entt::registry &registry, VulkanHelpers::UniformBuffer &uniformBuffer, vk::Extent2D extent);

} // namespace Systems

#endif // CAMERA_SYSTEM_H
