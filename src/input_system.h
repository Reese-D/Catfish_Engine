#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

#include <entt/entt.hpp>

#include "window.h"

namespace Systems {

void updateCameraInput(entt::registry &registry, VulkanHelpers::Window &window, float deltaTime);

} // namespace Systems

#endif // INPUT_SYSTEM_H
