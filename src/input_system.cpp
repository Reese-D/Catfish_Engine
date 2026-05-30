#include <algorithm>
#include <cmath>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "components.h"
#include "input_system.h"

namespace Systems {

void updateCameraInput(entt::registry &registry, VulkanHelpers::Window &window, float deltaTime) {
    constexpr float panSpeed  = 3.0f; // world units per second
    constexpr float zoomSpeed = 4.0f; // world units per scroll tick
    constexpr float minDist   = 1.0f;
    constexpr float maxDist   = 30.0f;

    auto view = registry.view<Components::Camera>();
    for (auto entity : view) {
        auto &cam = view.get<Components::Camera>(entity);

        // Pan — translate position and target together along world XY
        glm::vec3 pan{0.0f};
        if (window.isKeyPressed(GLFW_KEY_W) || window.isKeyPressed(GLFW_KEY_UP))    pan.y += panSpeed * deltaTime;
        if (window.isKeyPressed(GLFW_KEY_S) || window.isKeyPressed(GLFW_KEY_DOWN))  pan.y -= panSpeed * deltaTime;
        if (window.isKeyPressed(GLFW_KEY_A) || window.isKeyPressed(GLFW_KEY_LEFT))  pan.x -= panSpeed * deltaTime;
        if (window.isKeyPressed(GLFW_KEY_D) || window.isKeyPressed(GLFW_KEY_RIGHT)) pan.x += panSpeed * deltaTime;
        cam.position += pan;
        cam.target   += pan;

        // Zoom — move position along the arm toward/away from target
        float scroll = window.consumeScrollDelta();
        if (std::abs(scroll) > 0.0f) {
            glm::vec3 arm  = cam.position - cam.target;
            float    dist  = glm::length(arm);
            float    newDist = std::clamp(dist - scroll * zoomSpeed, minDist, maxDist);
            cam.position = cam.target + glm::normalize(arm) * newDist;
        }

        return; // only the first camera
    }
}

} // namespace Systems
