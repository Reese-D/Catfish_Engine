#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera_system.h"
#include "components.h"

namespace Systems {

void updateCamera(entt::registry &registry, vk::Extent2D extent) {
    auto camView = registry.view<Components::Camera>();
    for (auto entity : camView) {
        auto &cam = camView.get<Components::Camera>(entity);

        if (cam.followPlayer) {
            for (auto playerEntity : registry.view<Components::AlwaysSelected, Components::Transform>()) {
                const auto &pt = registry.get<Components::Transform>(playerEntity);
                glm::vec3 offset = cam.position - cam.target;
                cam.target = {pt.position.x, pt.position.y, 0.0f};
                cam.position = cam.target + offset;
                break;
            }
        }

        cam.view = glm::lookAt(cam.position, cam.target, glm::vec3(0.0f, 0.0f, 1.0f));
        cam.proj = glm::perspective(glm::radians(cam.fov), static_cast<float>(extent.width) / static_cast<float>(extent.height), cam.near_, cam.far_);
        cam.proj[1][1] *= -1; // Vulkan Y-flip
        return;
    }
}

} // namespace Systems
