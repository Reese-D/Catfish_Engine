#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera_system.h"
#include "components.h"

namespace Systems {

void updateCamera(entt::registry &registry, VulkanHelpers::UniformBuffer &uniformBuffer, vk::Extent2D extent) {
    auto view = registry.view<Components::Camera>();
    for (auto entity : view) {
        const auto &cam = view.get<Components::Camera>(entity);

        auto viewMatrix = glm::lookAt(cam.position, cam.target, glm::vec3(0.0f, 0.0f, 1.0f));
        auto projMatrix = glm::perspective(
            glm::radians(cam.fov),
            static_cast<float>(extent.width) / static_cast<float>(extent.height),
            cam.near_, cam.far_
        );
        projMatrix[1][1] *= -1; // Vulkan Y-flip

        uniformBuffer.update(viewMatrix, projMatrix);
        return; // only the first camera entity drives the view
    }
}

} // namespace Systems
