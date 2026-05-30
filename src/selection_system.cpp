#include <optional>
#include <vector>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "components.h"
#include "orders.h"
#include "selection_system.h"

namespace {

glm::vec3 screenToRayDir(glm::vec2 mousePos, glm::vec2 screenSize, const glm::mat4 &view, const glm::mat4 &proj) {
    // Vulkan NDC: X in [-1,1] left→right, Y in [-1,1] top→bottom, depth Z in [0,1]
    float ndcX = (2.0f * mousePos.x) / screenSize.x - 1.0f;
    float ndcY = (2.0f * mousePos.y) / screenSize.y - 1.0f;

    // Unproject two points on the ray (near Z=0, far Z=1) directly into world space
    glm::mat4 invVP = glm::inverse(proj * view);
    glm::vec4 nearW = invVP * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 farW  = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    nearW /= nearW.w;
    farW  /= farW.w;

    return glm::normalize(glm::vec3(farW) - glm::vec3(nearW));
}

std::optional<glm::vec3> rayGroundIntersect(glm::vec3 origin, glm::vec3 dir) {
    if (std::abs(dir.z) < 1e-6f) return std::nullopt;
    float t = -origin.z / dir.z;
    if (t < 0.0f) return std::nullopt;
    return origin + t * dir;
}

} // namespace

namespace Systems {

void updateSelection(entt::registry &registry, const VulkanHelpers::Window &window, vk::Extent2D extent) {
    static bool prevLeft  = false;
    static bool prevRight = false;

    bool leftDown  = window.isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
    bool rightDown = window.isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);
    bool leftJust  = leftDown  && !prevLeft;
    bool rightJust = rightDown && !prevRight;
    prevLeft  = leftDown;
    prevRight = rightDown;

    if (!leftJust && !rightJust) return;

    // Retrieve the active camera
    const Components::Camera *cam = nullptr;
    for (auto entity : registry.view<Components::Camera>()) {
        cam = &registry.get<Components::Camera>(entity);
        break;
    }
    if (!cam) return;

    auto [mx, my] = window.getMousePosition();
    glm::vec2 mousePos{static_cast<float>(mx), static_cast<float>(my)};
    glm::vec2 screenSize{static_cast<float>(extent.width), static_cast<float>(extent.height)};

    glm::vec3 rayDir = screenToRayDir(mousePos, screenSize, cam->view, cam->proj);

    // Left click — select the nearest Selectable entity to the pick ray
    if (leftJust) {
        entt::entity nearest     = entt::null;
        float        nearestDist = 0.5f; // world-unit selection radius

        for (auto entity : registry.view<Components::Transform, Components::Selectable>()) {
            const auto &t       = registry.get<Components::Transform>(entity);
            glm::vec3   toEntity = t.position - cam->position;
            float       dist     = glm::length(glm::cross(toEntity, rayDir));
            if (dist < nearestDist) {
                nearestDist = dist;
                nearest     = entity;
            }
        }

        // Collect then deselect to avoid modifying the view mid-iteration
        std::vector<entt::entity> toDeselect;
        for (auto entity : registry.view<Components::Selected>()) {
            toDeselect.push_back(entity);
        }
        for (auto entity : toDeselect) {
            registry.remove<Components::Selected>(entity);
        }

        if (nearest != entt::null) {
            registry.emplace<Components::Selected>(nearest);
        }
    }

    // Right click — issue MoveOrder to all Selected units at the ground point
    if (rightJust) {
        auto groundHit = rayGroundIntersect(cam->position, rayDir);
        if (!groundHit) return;

        for (auto entity : registry.view<Components::Selected, Components::OrderQueue>()) {
            auto &queue = registry.get<Components::OrderQueue>(entity);
            queue.enqueueImmediate(Orders::MoveOrder{*groundHit});
        }
    }
}

} // namespace Systems
