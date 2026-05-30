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

        result.push_back(VulkanHelpers::DrawCall{
            mesh.model.get(),
            mesh.model->getMaterial().getDescriptorSet(),
            modelMatrix,
        });
    }

    return result;
}

void appendSelectionRings(
    entt::registry &registry,
    std::vector<VulkanHelpers::DrawCall> &draws,
    const VulkanHelpers::Model &ringModel
) {
    vk::DescriptorSet matSet = ringModel.getMaterial().getDescriptorSet();

    for (auto entity : registry.view<Components::Selected, Components::Transform>()) {
        const auto &t = registry.get<Components::Transform>(entity);
        // Place ring flat on the ground at the unit's XY, just above terrain
        auto m = glm::translate(glm::mat4(1.0f), glm::vec3(t.position.x, t.position.y, 0.02f));
        draws.push_back({&ringModel, matSet, m});
    }
}

} // namespace Systems
