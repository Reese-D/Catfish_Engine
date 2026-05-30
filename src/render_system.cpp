#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "components.h"
#include "render_system.h"

namespace Systems {

std::vector<VulkanHelpers::DrawCall> collectDrawCalls(entt::registry &registry) {
    std::vector<VulkanHelpers::DrawCall> result;

    auto view = registry.view<Components::Transform, Components::RenderMesh>();
    for (auto entity : view) {
        const auto &transform = view.get<Components::Transform>(entity);
        const auto &mesh = view.get<Components::RenderMesh>(entity);

        auto modelMatrix = glm::translate(glm::mat4(1.0f), transform.position);
        modelMatrix = modelMatrix * glm::mat4_cast(transform.rotation);
        modelMatrix = glm::scale(modelMatrix, transform.scale);

        result.push_back(VulkanHelpers::DrawCall{mesh.model.get(), modelMatrix});
    }

    return result;
}

} // namespace Systems
