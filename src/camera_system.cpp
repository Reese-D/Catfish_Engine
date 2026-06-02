#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera_system.h"
#include "components.h"

namespace Systems {

void updateCamera(entt::registry &registry, vk::Extent2D extent) {
    auto view = registry.view<Components::Camera>();
    for (auto entity : view) {
        auto &cam = view.get<Components::Camera>(entity);

        cam.view = glm::lookAt(cam.position, cam.target, glm::vec3(0.0f, 0.0f, 1.0f));
        cam.proj = glm::perspective(glm::radians(cam.fov), static_cast<float>(extent.width) / static_cast<float>(extent.height), cam.near_, cam.far_);
        cam.proj[1][1] *= -1; // Vulkan Y-flip
        return;
    }
}

} // namespace Systems
