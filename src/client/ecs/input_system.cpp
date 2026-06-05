#include <algorithm>
#include <cmath>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "components.h"
#include "input_system.h"

namespace Systems {

void updateCameraInput(entt::registry &registry, VulkanHelpers::Window &window, float deltaTime) {
    constexpr float kPanSpeed = 3.0f;  // world units per second
    constexpr float kZoomSpeed = 4.0f; // world units per scroll tick
    constexpr float kMinDist = 1.0f;
    constexpr float kMaxDist = 30.0f;

    auto view = registry.view<Components::Camera>();
    for (auto entity : view) {
        auto &cam = view.get<Components::Camera>(entity);

        // Pan — WASD/arrow keys or mouse at screen edge
        constexpr float kEdgePx = 12.0f; // pixels from edge that trigger scroll
        glm::vec3 pan{0.0f};
        if (window.isKeyPressed(GLFW_KEY_W) || window.isKeyPressed(GLFW_KEY_UP))
            pan.y += kPanSpeed * deltaTime;
        if (window.isKeyPressed(GLFW_KEY_S) || window.isKeyPressed(GLFW_KEY_DOWN))
            pan.y -= kPanSpeed * deltaTime;
        if (window.isKeyPressed(GLFW_KEY_A) || window.isKeyPressed(GLFW_KEY_LEFT))
            pan.x -= kPanSpeed * deltaTime;
        if (window.isKeyPressed(GLFW_KEY_D) || window.isKeyPressed(GLFW_KEY_RIGHT))
            pan.x += kPanSpeed * deltaTime;

        auto [mx, my] = window.getMousePosition();
        float fw = static_cast<float>(window.getWidth());
        float fh = static_cast<float>(window.getHeight());
        if (mx < kEdgePx)
            pan.x -= kPanSpeed * deltaTime;
        if (mx > fw - kEdgePx)
            pan.x += kPanSpeed * deltaTime;
        if (my < kEdgePx)
            pan.y += kPanSpeed * deltaTime;
        if (my > fh - kEdgePx)
            pan.y -= kPanSpeed * deltaTime;

        cam.position += pan;
        cam.target += pan;

        // Zoom — move position along the arm toward/away from target
        float scroll = window.consumeScrollDelta();
        if (std::abs(scroll) > 0.0f) {
            glm::vec3 arm = cam.position - cam.target;
            float dist = glm::length(arm);
            float newDist = std::clamp(dist - scroll * kZoomSpeed, kMinDist, kMaxDist);
            cam.position = cam.target + glm::normalize(arm) * newDist;
        }

        return; // only the first camera
    }
}

} // namespace Systems
